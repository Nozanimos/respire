// SPDX-License-Identifier: GPL-3.0-or-later
#include "json_editor.h"
#include "../debug.h"
#include <string.h>

// ════════════════════════════════════════════════════════════════════════════
//  UTILITAIRES UTF8 POUR LA SÉLECTION
// ════════════════════════════════════════════════════════════════════════════

// Retourne le nombre d'octets d'un caractère UTF-8
static int utf8_char_len(const char* str) {
    unsigned char c = (unsigned char)*str;
    if (c < 0x80) return 1;        // ASCII: 0xxxxxxx
    if ((c & 0xE0) == 0xC0) return 2;  // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3;  // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4;  // 11110xxx
    return 1;  // Invalid UTF-8, traiter comme 1 octet
}

// Compte le nombre de caractères UTF-8 dans une chaîne
int utf8_strlen(const char* str) {
    int count = 0;
    while (*str) {
        str += utf8_char_len(str);
        count++;
    }
    return count;
}

// Avance de n caractères UTF-8 dans une chaîne, retourne le nombre d'octets avancés
int utf8_advance(const char* str, int n_chars) {
    int bytes = 0;
    for (int i = 0; i < n_chars && str[bytes]; i++) {
        bytes += utf8_char_len(str + bytes);
    }
    return bytes;
}

// Copie n caractères UTF-8 (pas n octets!) dans dest
void utf8_strncpy(char* dest, const char* src, int n_chars, int max_bytes) {
    int bytes_copied = 0;
    int chars_copied = 0;

    while (chars_copied < n_chars && src[bytes_copied] && bytes_copied < max_bytes - 1) {
        int char_len = utf8_char_len(src + bytes_copied);
        if (bytes_copied + char_len >= max_bytes) break;

        for (int i = 0; i < char_len; i++) {
            dest[bytes_copied] = src[bytes_copied];
            bytes_copied++;
        }
        chars_copied++;
    }
    dest[bytes_copied] = '\0';
}

// ════════════════════════════════════════════════════════════════════════════
//  AUTO-SAVE POUR HOT RELOAD
// ════════════════════════════════════════════════════════════════════════════
void marquer_modification(JsonEditor* editor) {
    if (!editor) return;
    editor->last_modification_time = SDL_GetTicks();
}

void verifier_auto_save(JsonEditor* editor) {
    if (!editor || !editor->modified) return;

    Uint32 now = SDL_GetTicks();
    float elapsed = (now - editor->last_modification_time) / 1000.0f;

    if (elapsed >= editor->auto_save_delay) {
        sauvegarder_fichier_json(editor);
        editor->last_modification_time = now;
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  DÉSÉLECTIONNER LE TEXTE
// ════════════════════════════════════════════════════════════════════════════
void deselectionner(JsonEditor* editor) {
    if (!editor) return;
    editor->selection_active = false;
    editor->selection_start = -1;
    editor->selection_end = -1;
}

// ════════════════════════════════════════════════════════════════════════════
//  INSERTION DE CARACTÈRE
// ════════════════════════════════════════════════════════════════════════════
void inserer_caractere(JsonEditor* editor, char c) {
    if (!editor) return;

    // Supprimer la sélection si elle est active
    if (editor->selection_active) {
        int sel_min = min_int(editor->selection_start, editor->selection_end);
        int sel_max = max_int(editor->selection_start, editor->selection_end);

        if (sel_min != sel_max) {
            // Supprimer le texte sélectionné
            int len = strlen(editor->buffer);
            memmove(&editor->buffer[sel_min],
                    &editor->buffer[sel_max],
                    len - sel_max + 1);

            // Placer le curseur à la position de début de la sélection
            editor->curseur_position = sel_min;
            debug_printf("✅ Sélection remplacée (de %d à %d)\n", sel_min, sel_max);
        }

        // Désélectionner
        deselectionner(editor);
    }

    int len = strlen(editor->buffer);

    if (len >= JSON_BUFFER_SIZE - 1) return;

    // LOG
    debug_printf("📝 INSERT: curseur_pos=%d, char_insere='%c'\n",
                 editor->curseur_position, c);

    // Décaler tous les caractères après le curseur
    memmove(&editor->buffer[editor->curseur_position + 1],
            &editor->buffer[editor->curseur_position],
            len - editor->curseur_position + 1);

    // Insérer le caractère
    editor->buffer[editor->curseur_position] = c;
    editor->curseur_position++;
    editor->modified = true;
    marquer_modification(editor);
    editor->nb_lignes = compter_lignes(editor->buffer);

    // LOG
    debug_printf("✅ Après insertion: nouveau_curseur_pos=%d\n", editor->curseur_position);
}


// ─────────────────────────────────────────────────────────────────────────
//  NOUVELLE FONCTIONNALITÉ : PRESSE-PAPIER (CLIPBOARD)
// ─────────────────────────────────────────────────────────────────────────

// AJOUTER dans la structure JsonEditor (dans le .h et au début du .c) :
//
//     char clipboard[JSON_BUFFER_SIZE];  // Presse-papier interne
//

// AJOUTER ces nouvelles fonctions dans json_editor_window.c :



// ════════════════════════════════════════════════════════════════════════════
//  SUPPRESSION DE CARACTÈRE (BACKSPACE)
// ════════════════════════════════════════════════════════════════════════════
void supprimer_caractere(JsonEditor* editor) {
    if (!editor) return;

    // ✅ CORRECTION : Sauvegarder l'état AVANT toute modification
    sauvegarder_etat_undo(editor);

    // Si une sélection est active, supprimer toute la sélection
    if (editor->selection_active) {
        int sel_min = min_int(editor->selection_start, editor->selection_end);
        int sel_max = max_int(editor->selection_start, editor->selection_end);

        if (sel_min != sel_max) {
            // Supprimer tout le texte entre sel_min et sel_max
            int len = strlen(editor->buffer);
            memmove(&editor->buffer[sel_min],
                    &editor->buffer[sel_max],
                    len - sel_max + 1);

            editor->curseur_position = sel_min;
            editor->modified = true;
    marquer_modification(editor);
            editor->nb_lignes = compter_lignes(editor->buffer);

            deselectionner(editor);
            debug_printf("✅ Sélection supprimée (de %d à %d)\n", sel_min, sel_max);
            return;
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // SUPPRESSION NORMALE D'UN SEUL CARACTÈRE (UTF-8 AWARE)
    // ═════════════════════════════════════════════════════════════════════════

    debug_printf("🔍 [BACKSPACE] Début - curseur_pos=%d\n", editor->curseur_position);

    if (editor->curseur_position <= 0) {
        debug_printf("⚠️ [BACKSPACE] Curseur au début du buffer, rien à supprimer\n");
        return;
    }

    int pos = editor->curseur_position;

    // Afficher le contexte avant suppression
    if (pos > 0) {
        debug_printf("🔍 [BACKSPACE] Caractère avant curseur: 0x%02X '%c'\n",
                     (unsigned char)editor->buffer[pos-1],
                     (editor->buffer[pos-1] >= 32 && editor->buffer[pos-1] < 127) ? editor->buffer[pos-1] : '?');
    }

    // ─────────────────────────────────────────────────────────────────────────
    // TROUVER LE DÉBUT DU CARACTÈRE UTF-8 À SUPPRIMER
    // ─────────────────────────────────────────────────────────────────────────
    int char_start = pos - 1;

    // Si on est sur un octet de continuation (10xxxxxx), reculer jusqu'au début
    while (char_start > 0 && (editor->buffer[char_start] & 0xC0) == 0x80) {
        char_start--;
        debug_printf("🔍 [BACKSPACE] Recul UTF-8: char_start maintenant à %d\n", char_start);
    }

    // Calculer le nombre d'octets du caractère à supprimer
    int char_len = pos - char_start;

    debug_printf("🔍 [BACKSPACE] Caractère UTF-8 détecté:\n");
    debug_printf("              - Position début: %d\n", char_start);
    debug_printf("              - Position fin: %d\n", pos);
    debug_printf("              - Longueur: %d octets\n", char_len);

    // Afficher les octets du caractère
    debug_printf("              - Octets: ");
    for (int i = char_start; i < pos; i++) {
        debug_printf("0x%02X ", (unsigned char)editor->buffer[i]);
    }
    debug_printf("\n");

    // ─────────────────────────────────────────────────────────────────────────
    // SUPPRIMER LE CARACTÈRE
    // ─────────────────────────────────────────────────────────────────────────
    int buffer_len = strlen(editor->buffer);

    debug_printf("🔍 [BACKSPACE] memmove: dest=%d, src=%d, len=%d\n",
                 char_start, pos, buffer_len - pos + 1);

    memmove(editor->buffer + char_start,
            editor->buffer + pos,
            buffer_len - pos + 1);  // +1 pour le '\0'

            // Mettre à jour le curseur
            editor->curseur_position = char_start;
            editor->modified = true;
    marquer_modification(editor);
            editor->nb_lignes = compter_lignes(editor->buffer);

            debug_printf("✅ [BACKSPACE] Caractère supprimé! Nouveau curseur_pos=%d\n",
                         editor->curseur_position);
            debug_printf("✅ [BACKSPACE] Buffer après suppression: longueur=%d\n",
                         (int)strlen(editor->buffer));
}

// ════════════════════════════════════════════════════════════════════════════
//  SUPPRESSION DE CARACTÈRE APRÈS LE CURSEUR (DELETE)
// ════════════════════════════════════════════════════════════════════════════
void supprimer_caractere_apres(JsonEditor* editor) {
    if (!editor) return;

    // ✅ Sauvegarder l'état AVANT toute modification
    sauvegarder_etat_undo(editor);

    // Si une sélection est active, supprimer toute la sélection
    if (editor->selection_active) {
        int sel_min = min_int(editor->selection_start, editor->selection_end);
        int sel_max = max_int(editor->selection_start, editor->selection_end);

        if (sel_min != sel_max) {
            // Supprimer tout le texte entre sel_min et sel_max
            int len = strlen(editor->buffer);
            memmove(&editor->buffer[sel_min],
                    &editor->buffer[sel_max],
                    len - sel_max + 1);

            editor->curseur_position = sel_min;
            editor->modified = true;
    marquer_modification(editor);
            editor->nb_lignes = compter_lignes(editor->buffer);

            deselectionner(editor);
            debug_printf("✅ [DELETE] Sélection supprimée (de %d à %d)\n", sel_min, sel_max);
            return;
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // SUPPRESSION NORMALE D'UN SEUL CARACTÈRE APRÈS LE CURSEUR (UTF-8 AWARE)
    // ═════════════════════════════════════════════════════════════════════════

    debug_printf("🔍 [DELETE] Début - curseur_pos=%d\n", editor->curseur_position);

    int buffer_len = strlen(editor->buffer);
    if (editor->curseur_position >= buffer_len) {
        debug_printf("⚠️ [DELETE] Curseur à la fin du buffer, rien à supprimer\n");
        return;
    }

    int pos = editor->curseur_position;

    // Afficher le contexte avant suppression
    debug_printf("🔍 [DELETE] Caractère après curseur: 0x%02X '%c'\n",
                 (unsigned char)editor->buffer[pos],
                 (editor->buffer[pos] >= 32 && editor->buffer[pos] < 127) ? editor->buffer[pos] : '?');

    // ─────────────────────────────────────────────────────────────────────────
    // TROUVER LA LONGUEUR DU CARACTÈRE UTF-8 À SUPPRIMER
    // ─────────────────────────────────────────────────────────────────────────
    int char_len = utf8_char_len(editor->buffer + pos);

    debug_printf("🔍 [DELETE] Caractère UTF-8 détecté:\n");
    debug_printf("              - Position début: %d\n", pos);
    debug_printf("              - Position fin: %d\n", pos + char_len);
    debug_printf("              - Longueur: %d octets\n", char_len);

    // Afficher les octets du caractère
    debug_printf("              - Octets: ");
    for (int i = pos; i < pos + char_len && i < buffer_len; i++) {
        debug_printf("0x%02X ", (unsigned char)editor->buffer[i]);
    }
    debug_printf("\n");

    // ─────────────────────────────────────────────────────────────────────────
    // SUPPRIMER LE CARACTÈRE
    // ─────────────────────────────────────────────────────────────────────────
    debug_printf("🔍 [DELETE] memmove: dest=%d, src=%d, len=%d\n",
                 pos, pos + char_len, buffer_len - pos - char_len + 1);

    memmove(editor->buffer + pos,
            editor->buffer + pos + char_len,
            buffer_len - pos - char_len + 1);  // +1 pour le '\0'

            // Le curseur reste à la même position
            editor->modified = true;
    marquer_modification(editor);
            editor->nb_lignes = compter_lignes(editor->buffer);

            debug_printf("✅ [DELETE] Caractère supprimé! Curseur_pos reste à %d\n",
                         editor->curseur_position);
            debug_printf("✅ [DELETE] Buffer après suppression: longueur=%d\n",
                         (int)strlen(editor->buffer));
}

// ════════════════════════════════════════════════════════════════════════════
//  INSERTION NOUVELLE LIGNE
// ════════════════════════════════════════════════════════════════════════════
void inserer_nouvelle_ligne(JsonEditor* editor) {
    inserer_caractere(editor, '\n');
}

// ════════════════════════════════════════════════════════════════════════════
//  UTILITAIRES
// ════════════════════════════════════════════════════════════════════════════
int compter_lignes(const char* buffer) {
    if (!buffer) return 0;
    int count = 1;
    for (const char* p = buffer; *p; p++) {
        if (*p == '\n') count++;
    }
    return count;
}

const char* obtenir_ligne(const char* buffer, int index_ligne, char* dest, int max_len) {
    if (!buffer || !dest || index_ligne < 0) {
        dest[0] = '\0';
        return dest;
    }

    const char* start = buffer;
    int current_line = 0;

    // Trouver le début de la ligne
    while (*start && current_line < index_ligne) {
        if (*start == '\n') current_line++;
        start++;
    }

    // Copier jusqu'au \n ou fin
    int i = 0;
    while (*start && *start != '\n' && i < max_len - 1) {
        dest[i++] = *start++;
    }
    dest[i] = '\0';

    return dest;
}

// ════════════════════════════════════════════════════════════════════════════
//  DÉTECTION ET MANIPULATION DE NOMBRES
// ════════════════════════════════════════════════════════════════════════════

// Vérifie si un caractère fait partie d'un nombre JSON
static bool est_char_nombre(char c) {
    return isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E';
}

// Trouve les limites d'un nombre autour du curseur
// Retourne true si un nombre a été trouvé
bool trouver_nombre_au_curseur(JsonEditor* editor, int* debut, int* fin) {
    if (!editor || !debut || !fin) return false;

    int pos = editor->curseur_position;

    // Si on est après la fin du buffer, reculer d'un caractère
    if (pos >= (int)strlen(editor->buffer)) {
        pos--;
    }

    // Si on n'est pas sur un nombre, vérifier le caractère avant
    if (pos >= 0 && !est_char_nombre(editor->buffer[pos])) {
        if (pos > 0 && est_char_nombre(editor->buffer[pos - 1])) {
            pos--;
        } else {
            return false;  // Pas sur un nombre
        }
    }

    // Si on arrive ici et qu'on n'est toujours pas sur un nombre, exit
    if (pos < 0 || !est_char_nombre(editor->buffer[pos])) {
        return false;
    }

    // Trouver le début du nombre (reculer)
    int start = pos;
    while (start > 0 && est_char_nombre(editor->buffer[start - 1])) {
        start--;
    }

    // Trouver la fin du nombre (avancer)
    int end = pos;
    int buffer_len = strlen(editor->buffer);
    while (end < buffer_len && est_char_nombre(editor->buffer[end])) {
        end++;
    }

    *debut = start;
    *fin = end;
    return true;
}

// Incrémente ou décrémente un nombre dans le buffer
void modifier_nombre_au_curseur(JsonEditor* editor, int delta) {
    if (!editor) return;

    int debut, fin;
    if (!trouver_nombre_au_curseur(editor, &debut, &fin)) {
        return;  // Pas de nombre sous le curseur
    }

    // Sauvegarder pour undo
    sauvegarder_etat_undo(editor);

    // Extraire le nombre
    char nombre_str[64];
    int len = fin - debut;
    if (len >= 64) len = 63;
    strncpy(nombre_str, editor->buffer + debut, len);
    nombre_str[len] = '\0';

    // Parser le nombre
    bool est_decimal = (strchr(nombre_str, '.') != NULL ||
    strchr(nombre_str, 'e') != NULL ||
    strchr(nombre_str, 'E') != NULL);

    char nouveau_nombre[64];

    if (est_decimal) {
        // Nombre à virgule
        double valeur = atof(nombre_str);
        valeur += delta * 0.1;  // Incrément de 0.1 pour les décimaux

        // Garder le même nombre de décimales si possible
        int nb_decimales = 1;
        char* point = strchr(nombre_str, '.');
        if (point) {
            nb_decimales = 0;
            point++;
            while (*point && isdigit(*point)) {
                nb_decimales++;
                point++;
            }
        }

        snprintf(nouveau_nombre, sizeof(nouveau_nombre), "%.*f", nb_decimales, valeur);
    } else {
        // Nombre entier
        int valeur = atoi(nombre_str);
        valeur += delta;
        snprintf(nouveau_nombre, sizeof(nouveau_nombre), "%d", valeur);
    }

    // Remplacer le nombre dans le buffer
    int nouvelle_len = strlen(nouveau_nombre);
    int ancienne_len = fin - debut;
    int buffer_len = strlen(editor->buffer);

    // Décaler le reste du buffer si les longueurs diffèrent
    if (nouvelle_len != ancienne_len) {
        int diff = nouvelle_len - ancienne_len;

        // Vérifier qu'on a assez de place
        if (buffer_len + diff >= JSON_BUFFER_SIZE - 1) {
            return;  // Pas assez de place
        }

        // Décaler
        memmove(editor->buffer + fin + diff,
                editor->buffer + fin,
                buffer_len - fin + 1);  // +1 pour le '\0'
    }

    // Copier le nouveau nombre
    memcpy(editor->buffer + debut, nouveau_nombre, nouvelle_len);

    // Mettre à jour le curseur (le placer après le nombre)
    editor->curseur_position = debut + nouvelle_len;

    editor->modified = true;
    marquer_modification(editor);
    editor->nb_lignes = compter_lignes(editor->buffer);

    debug_printf("🔢 Nombre modifié: '%s' → '%s' (delta: %d)\n",
                 nombre_str, nouveau_nombre, delta);
}

// ════════════════════════════════════════════════════════════════════════════
//  SÉLECTION DE MOT ET DE LIGNE (UTF-8 AWARE)
// ════════════════════════════════════════════════════════════════════════════

// Retourne la longueur en octets d'un caractère UTF-8 à la position donnée
static int longueur_char_utf8_a_pos(const char* buffer, int pos) {
    if (!buffer || pos < 0) return 1;

    unsigned char c = (unsigned char)buffer[pos];
    if (c < 0x80) return 1;        // ASCII: 0xxxxxxx
    if ((c & 0xE0) == 0xC0) return 2;  // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3;  // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4;  // 11110xxx
    return 1;  // Défaut (invalide)
}

// Recule d'un caractère UTF-8 complet
// Retourne la nouvelle position
static int reculer_un_char_utf8(const char* buffer, int pos) {
    if (pos <= 0) return 0;

    pos--;  // Reculer d'au moins 1

    // Si on est sur un octet de continuation (10xxxxxx), reculer jusqu'au début
    while (pos > 0 && (buffer[pos] & 0xC0) == 0x80) {
        pos--;
    }

    return pos;
}

// Avance d'un caractère UTF-8 complet
// Retourne la nouvelle position
static int avancer_un_char_utf8(const char* buffer, int pos, int buffer_len) {
    if (pos >= buffer_len) return buffer_len;

    int char_len = longueur_char_utf8_a_pos(buffer, pos);
    pos += char_len;

    if (pos > buffer_len) pos = buffer_len;
    return pos;
}

// Vérifie si un caractère UTF-8 fait partie d'un "mot"
// Pour ça on teste :
// - Les ASCII alphanumériques classiques
// - Les caractères multi-octets (lettres accentuées, etc.) sont acceptés par défaut
// - On exclut les ponctuations communes et les espaces
static bool est_caractere_de_mot_utf8(const char* buffer, int pos, int buffer_len) {
    if (pos < 0 || pos >= buffer_len) return false;

    unsigned char c = (unsigned char)buffer[pos];

    // ─────────────────────────────────────────────────────────────────────────
    // Caractères ASCII : test précis
    // ─────────────────────────────────────────────────────────────────────────
    if (c < 0x80) {
        // Lettres et chiffres
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            return true;
        }
        // Underscore et tiret (communs dans les identifiants)
        if (c == '_' || c == '-') {
            return true;
        }
        // Tout le reste (espaces, ponctuation, etc.) n'est pas un mot
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Caractères multi-octets UTF-8
    // ─────────────────────────────────────────────────────────────────────────
    // On pourrait faire une détection fine des plages Unicode, mais c'est complexe.
    // Approche pragmatique : on accepte les caractères multi-octets SAUF :
    // - Les guillemets typographiques « » " "
    // - Les espaces insécables et autres espaces Unicode

    // Vérifier si c'est le début d'un caractère multi-octets valide
    if ((c & 0xE0) == 0xC0 || (c & 0xF0) == 0xE0 || (c & 0xF8) == 0xF0) {
        // Pour l'instant, on accepte tous les caractères multi-octets
        // Ça couvre : é, à, ç, œ, €, emojis, etc.
        // Si tu veux filtrer plus finement, on peut ajouter des exclusions ici
        return true;
    }

    return false;
}

// Sélectionne le mot sous le curseur (UTF-8 aware)
void selectionner_mot_au_curseur(JsonEditor* editor) {
    if (!editor) return;

    int pos = editor->curseur_position;
    int buffer_len = strlen(editor->buffer);

    if (pos < 0 || pos >= buffer_len) return;

    // ═════════════════════════════════════════════════════════════════════════
    // Sélectionner le mot sous le curseur (avec support UTF-8)
    // ═════════════════════════════════════════════════════════════════════════

    // Vérifier qu'on est sur un caractère de mot
    if (!est_caractere_de_mot_utf8(editor->buffer, pos, buffer_len)) {
        // Essayer le caractère avant
        int pos_avant = reculer_un_char_utf8(editor->buffer, pos);
        if (pos_avant < pos && est_caractere_de_mot_utf8(editor->buffer, pos_avant, buffer_len)) {
            pos = pos_avant;
        } else {
            // Pas sur un mot, peut-être qu'on est sur un guillemet ou ponctuation
            // Dans ce cas, ne rien sélectionner
            debug_printf("⚠️ Pas sur un mot, abandon de sélection\n");
            return;
        }
    }

    // Trouver le début du mot (reculer caractère par caractère)
    int debut = pos;
    while (debut > 0) {
        int pos_avant = reculer_un_char_utf8(editor->buffer, debut);
        if (!est_caractere_de_mot_utf8(editor->buffer, pos_avant, buffer_len)) {
            break;  // On a atteint le début du mot
        }
        debut = pos_avant;
    }

    // Trouver la fin du mot (avancer caractère par caractère)
    int fin = pos;
    while (fin < buffer_len) {
        if (!est_caractere_de_mot_utf8(editor->buffer, fin, buffer_len)) {
            break;  // On a atteint la fin du mot
        }
        fin = avancer_un_char_utf8(editor->buffer, fin, buffer_len);
    }

    // Sélectionner le mot
    editor->selection_start = debut;
    editor->selection_end = fin;
    editor->selection_active = true;
    editor->curseur_position = fin;

    debug_printf("📝 MOT UTF-8 sélectionné: pos %d à %d\n", debut, fin);
}

// Sélectionne toute la ligne courante
void selectionner_ligne_courante(JsonEditor* editor) {
    if (!editor) return;

    int pos = editor->curseur_position;
    int buffer_len = strlen(editor->buffer);

    // Trouver le début de la ligne (reculer jusqu'au \n précédent ou début)
    int debut = pos;
    while (debut > 0 && editor->buffer[debut - 1] != '\n') {
        debut--;
    }

    // Trouver la fin de la ligne (avancer jusqu'au \n suivant ou fin)
    int fin = pos;
    while (fin < buffer_len && editor->buffer[fin] != '\n') {
        fin++;
    }

    // Inclure le \n final si présent
    if (fin < buffer_len && editor->buffer[fin] == '\n') {
        fin++;
    }

    // Sélectionner la ligne
    editor->selection_start = debut;
    editor->selection_end = fin;
    editor->selection_active = true;
    editor->curseur_position = fin;

    debug_printf("📝 LIGNE sélectionnée: pos %d à %d\n", debut, fin);
}

// ════════════════════════════════════════════════════════════════════════════
//  DUPLICATION DE LIGNE
// ════════════════════════════════════════════════════════════════════════════
void dupliquer_ligne_courante(JsonEditor* editor) {
    if (!editor) return;

    // Sauvegarder pour undo
    sauvegarder_etat_undo(editor);

    int buffer_len = strlen(editor->buffer);
    int pos = editor->curseur_position;

    // ─────────────────────────────────────────────────────────────────────────
    // Trouver le début de la ligne courante
    // ─────────────────────────────────────────────────────────────────────────
    int debut_ligne = pos;
    while (debut_ligne > 0 && editor->buffer[debut_ligne - 1] != '\n') {
        debut_ligne--;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Trouver la fin de la ligne courante
    // ─────────────────────────────────────────────────────────────────────────
    int fin_ligne = pos;
    while (fin_ligne < buffer_len && editor->buffer[fin_ligne] != '\n') {
        fin_ligne++;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Calculer la longueur de la ligne (sans le \n final)
    // ─────────────────────────────────────────────────────────────────────────
    int longueur_ligne = fin_ligne - debut_ligne;

    // ─────────────────────────────────────────────────────────────────────────
    // Vérifier qu'on a assez de place dans le buffer
    // ─────────────────────────────────────────────────────────────────────────
    // On va ajouter : la ligne + \n (donc longueur_ligne + 1)
    if (buffer_len + longueur_ligne + 1 >= JSON_BUFFER_SIZE - 1) {
        debug_printf("❌ Pas assez de place pour dupliquer la ligne\n");
        return;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Copier la ligne dans un buffer temporaire
    // ─────────────────────────────────────────────────────────────────────────
    char ligne_temp[JSON_BUFFER_SIZE];
    strncpy(ligne_temp, editor->buffer + debut_ligne, longueur_ligne);
    ligne_temp[longueur_ligne] = '\0';

    // ─────────────────────────────────────────────────────────────────────────
    // Insérer la ligne dupliquée juste après la ligne courante
    // ─────────────────────────────────────────────────────────────────────────
    // Position d'insertion : après le \n de la ligne courante (ou à fin_ligne si pas de \n)
    int pos_insertion = fin_ligne;
    if (fin_ligne < buffer_len && editor->buffer[fin_ligne] == '\n') {
        pos_insertion = fin_ligne + 1;
    }

    // Décaler tout ce qui est après pour faire de la place
    memmove(editor->buffer + pos_insertion + longueur_ligne + 1,
            editor->buffer + pos_insertion,
            buffer_len - pos_insertion + 1);  // +1 pour le '\0'

            // Insérer la ligne dupliquée
            memcpy(editor->buffer + pos_insertion, ligne_temp, longueur_ligne);

            // Ajouter un \n après la ligne dupliquée
            editor->buffer[pos_insertion + longueur_ligne] = '\n';

            // ─────────────────────────────────────────────────────────────────────────
            // Déplacer le curseur au début de la ligne dupliquée
            // ─────────────────────────────────────────────────────────────────────────
            editor->curseur_position = pos_insertion;

            editor->modified = true;
    marquer_modification(editor);
            editor->nb_lignes = compter_lignes(editor->buffer);

            debug_printf("📋 Ligne dupliquée: pos %d, longueur %d\n", debut_ligne, longueur_ligne);
}
