#!/bin/bash
# Script pour debugger le hovering des widgets INCREMENT
# Lance GDB avec le script de debug

echo "=========================================="
echo "Debug Hovering - Widgets INCREMENT"
echo "=========================================="
echo ""
echo "Compilation avec symboles de debug (-g)..."
make clean
make

if [ ! -f bin/respire ]; then
    echo "ERREUR: Compilation échouée"
    exit 1
fi

echo ""
echo "Lancement de GDB avec le script de debug..."
echo ""
echo "Instructions:"
echo "  1. Le programme va démarrer automatiquement"
echo "  2. Ouvre le panneau de configuration (Ctrl+S)"
echo "  3. Survole les widgets INCREMENT avec la souris"
echo "  4. Observe la sortie GDB pour voir les valeurs"
echo "  5. Ctrl+C pour arrêter"
echo ""
echo "=========================================="
echo ""

gdb -x debug_hover.gdb ./bin/respire
