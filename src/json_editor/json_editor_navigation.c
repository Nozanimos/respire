#include "json_editor.h"
#include "../debug.h"


// ════════════════════════════════════════════════════════════════════════════
//  DÉPLACEMENT VERTICAL DU CURSEUR
// ════════════════════════════════════════════════════════════════════════════
void deplacer_curseur_vertical(JsonEditor* editor, int direction) {
    if (!editor) return;

    // Calculer ligne et colonne actuelles
    int ligne_actuelle = 0;
    int colonne_actuelle = 0;

    for (int i = 0; i < editor->curseur_position; i++) {
        if (editor->buffer[i] == '\n') {
            ligne_actuelle++;
            colonne_actuelle = 0;
        } else {
            colonne_actuelle++;
        }
    }

    // Ligne cible
    int ligne_cible = ligne_actuelle + direction;
    if (ligne_cible < 0) {
        ligne_cible = 0;
        colonne_actuelle = 0; // Si on remonte en haut, on va au début
    }
    if (ligne_cible >= editor->nb_lignes) {
        ligne_cible = editor->nb_lignes - 1;
    }

    // ✅ FIX : Trouver le début de la ligne cible CORRECTEMENT
    int pos = 0;
    int ligne_courante = 0;

    // Parcourir jusqu'à la ligne cible
    while (editor->buffer[pos] && ligne_courante < ligne_cible) {
        if (editor->buffer[pos] == '\n') {
            ligne_courante++;
        }
        pos++;
    }

    // Maintenant pos est au DÉBUT de la ligne cible
    // Avancer sur la ligne jusqu'à la colonne souhaitée (ou fin de ligne)
    int col = 0;
    while (col < colonne_actuelle &&
        editor->buffer[pos] &&
        editor->buffer[pos] != '\n') {
        pos++;
    col++;
        }

        editor->curseur_position = pos;

        // ✅ LOG pour vérifier
        debug_printf("🔧 VERTICAL: ligne_actuelle=%d, ligne_cible=%d, col=%d, nouvelle_pos=%d\n",
                     ligne_actuelle, ligne_cible, colonne_actuelle, pos);
}

// ════════════════════════════════════════════════════════════════════════════
//  AUTO-SCROLL POUR SUIVRE LE CURSEUR (VERTICAL ET HORIZONTAL)
// ════════════════════════════════════════════════════════════════════════════
void auto_scroll_curseur(JsonEditor* editor) {
    if (!editor) return;

    // ─────────────────────────────────────────────────────────────────────────
    // SCROLL VERTICAL
    // ─────────────────────────────────────────────────────────────────────────
    int curseur_ligne = 0;
    int curseur_colonne = 0;
    for (int i = 0; i < editor->curseur_position && editor->buffer[i]; i++) {
        if (editor->buffer[i] == '\n') {
            curseur_ligne++;
            curseur_colonne = 0;
        } else {
            curseur_colonne++;
        }
    }

    int nb_lignes_visibles = (EDITOR_HEIGHT - 100) / LINE_HEIGHT;

    // Si le curseur est au-dessus de la zone visible, scroller vers le haut
    if (curseur_ligne < editor->scroll_offset) {
        editor->scroll_offset = curseur_ligne;
        debug_printf("⬆️ AUTO-SCROLL HAUT: ligne=%d\n", curseur_ligne);
    }

    // Si le curseur est en-dessous de la zone visible, scroller vers le bas
    if (curseur_ligne >= editor->scroll_offset + nb_lignes_visibles) {
        editor->scroll_offset = curseur_ligne - nb_lignes_visibles + 1;
        debug_printf("⬇️ AUTO-SCROLL BAS: ligne=%d\n", curseur_ligne);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SCROLL HORIZONTAL
    // ─────────────────────────────────────────────────────────────────────────
    // Seulement si on a une police chargée
    if (!editor->font_mono) return;

    // ═════════════════════════════════════════════════════════════════════════
    // CALCUL UTF-8 AWARE DE LA POSITION DU CURSEUR
    // ═════════════════════════════════════════════════════════════════════════

    // 1. Obtenir la ligne actuelle
    char ligne_actuelle[256];
    obtenir_ligne(editor->buffer, curseur_ligne, ligne_actuelle, sizeof(ligne_actuelle));

    // 2. Trouver la position du curseur EN OCTETS sur cette ligne
    int pos = 0;
    int ligne_courante = 0;

    // Aller au début de la ligne actuelle
    while (editor->buffer[pos] && ligne_courante < curseur_ligne) {
        if (editor->buffer[pos] == '\n') ligne_courante++;
        pos++;
    }

    // pos est maintenant au début de la ligne actuelle
    int debut_ligne = pos;

    // Calculer le nombre d'OCTETS entre le début de ligne et le curseur
    int octets_avant_curseur = editor->curseur_position - debut_ligne;

    // 3. Extraire le texte AVANT le curseur (en octets!)
    char texte_avant_curseur[256];
    if (octets_avant_curseur > 0 && octets_avant_curseur < 255) {
        strncpy(texte_avant_curseur, ligne_actuelle, octets_avant_curseur);
        texte_avant_curseur[octets_avant_curseur] = '\0';
    } else {
        texte_avant_curseur[0] = '\0';
    }

    // 4. Calculer la largeur visuelle du texte avant le curseur
    int largeur_texte = 0;
    if (octets_avant_curseur > 0) {
        TTF_SizeUTF8(editor->font_mono, texte_avant_curseur, &largeur_texte, NULL);
    }

    // Position ABSOLUE du curseur (sans tenir compte du scroll)
    int curseur_x_absolu = largeur_texte;

    // Position du curseur à l'ÉCRAN (avec le scroll actuel)
    int curseur_x_ecran = LEFT_MARGIN + curseur_x_absolu - editor->scroll_offset_x;

    // Largeur de la zone de texte visible
    int largeur_zone_texte = EDITOR_WIDTH - LEFT_MARGIN - 20;  // 20px de marge droite

    // Marge de confort : on laisse 80 pixels avant de scroller
    const int MARGE_CONFORT = 80;

    // ═════════════════════════════════════════════════════════════════════════
    // CAS 1 : Le curseur sort à DROITE → on scroll vers la droite
    // ═════════════════════════════════════════════════════════════════════════
    if (curseur_x_ecran > EDITOR_WIDTH - MARGE_CONFORT) {
        // Décaler le scroll pour ramener le curseur à une position confortable
        editor->scroll_offset_x = curseur_x_absolu - largeur_zone_texte + MARGE_CONFORT;
        debug_printf("➡️ AUTO-SCROLL DROITE: curseur_abs=%d, offset_x=%d\n",
                     curseur_x_absolu, editor->scroll_offset_x);
    }

    // ═════════════════════════════════════════════════════════════════════════
    // CAS 2 : Le curseur sort à GAUCHE → on scroll vers la gauche
    // ═════════════════════════════════════════════════════════════════════════
    if (curseur_x_ecran < LEFT_MARGIN + MARGE_CONFORT) {
        // Décaler le scroll pour ramener le curseur à une position confortable
        editor->scroll_offset_x = curseur_x_absolu - MARGE_CONFORT;
        if (editor->scroll_offset_x < 0) editor->scroll_offset_x = 0;
        debug_printf("⬅️ AUTO-SCROLL GAUCHE: curseur_abs=%d, offset_x=%d\n",
                     curseur_x_absolu, editor->scroll_offset_x);
    }
}
