#ifndef CONSTANTS_H
#define CONSTANTS_H

/* ═══════════════════════════════════════════════════════════════════════════
 * CONSTANTES GÉOMÉTRIQUES
 * ═══════════════════════════════════════════════════════════════════════════ */
#define NB_SIDE 6  /* Nombre de côtés de l'hexagone */

/* ═══════════════════════════════════════════════════════════════════════════
 * CONSTANTES MATHÉMATIQUES
 * ═══════════════════════════════════════════════════════════════════════════ */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * DIMENSIONS DE RÉFÉRENCE (Résolution HD Ready)
 * ═══════════════════════════════════════════════════════════════════════════ */
#define REFERENCE_WIDTH  1280
#define REFERENCE_HEIGHT 720

/* ═══════════════════════════════════════════════════════════════════════════
 * DIMENSIONS UI - PANNEAUX
 * ═══════════════════════════════════════════════════════════════════════════ */
#define BASE_PANEL_WIDTH  500  /* Largeur de référence du panneau de réglages */
#define JSON_EDITOR_WIDTH  600 /* Largeur de l'éditeur JSON */
#define JSON_EDITOR_HEIGHT 800 /* Hauteur de l'éditeur JSON */

/* ═══════════════════════════════════════════════════════════════════════════
 * SEUILS ET LIMITES UI
 * ═══════════════════════════════════════════════════════════════════════════ */
#define MOBILE_WIDTH_THRESHOLD 600  /* En dessous = mode mobile (pleine largeur) */
#define PANEL_LAYOUT_THRESHOLD 350  /* En dessous = layout en colonne */
#define MIN_PANEL_HEIGHT       400  /* Hauteur minimale du panneau */
#define ABSOLUTE_MIN_HEIGHT    200  /* Hauteur minimale absolue */

/* ═══════════════════════════════════════════════════════════════════════════
 * DIMENSIONS UI - ÉLÉMENTS
 * ═══════════════════════════════════════════════════════════════════════════ */
#define BASE_PREVIEW_SIZE 100  /* Taille de base des aperçus */
#define TITLE_WIDTH       200  /* Largeur par défaut des titres */
#define TITLE_HEIGHT       60  /* Hauteur par défaut des titres */
#define IMAGE_SIZE        250  /* Taille par défaut des images */
#define VERTICAL_MARGIN    20  /* Marge verticale entre éléments */

/* ═══════════════════════════════════════════════════════════════════════════
 * LIMITES DU FACTEUR D'ÉCHELLE
 * ═══════════════════════════════════════════════════════════════════════════ */
#define MIN_SCALE 0.3f   /* Très petits écrans (smartwatches, etc.) */
#define MAX_SCALE 3.0f   /* Très grands écrans (4K+) */

#endif /* CONSTANTS_H */
