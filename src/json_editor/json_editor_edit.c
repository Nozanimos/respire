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
