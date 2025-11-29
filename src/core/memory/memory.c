// SPDX-License-Identifier: GPL-3.0-or-later
#include "memory.h"
#include "core/debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Structure pour tracker une allocation
typedef struct AllocationRecord {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    struct AllocationRecord* next;
} AllocationRecord;

// État global du système de tracking
static struct {
    bool tracking_enabled;
    AllocationRecord* allocations;
    size_t allocation_count;
    size_t total_allocated;
    size_t total_freed;
    size_t peak_memory;
} memory_state = {
    .tracking_enabled = false,
    .allocations = NULL,
    .allocation_count = 0,
    .total_allocated = 0,
    .total_freed = 0,
    .peak_memory = 0
};

void memory_enable_tracking(bool enable) {
    memory_state.tracking_enabled = enable;
    if (enable) {
        debug_printf("🔍 Memory tracking activé\n");
    }
}

static void track_allocation(void* ptr, size_t size, const char* file, int line) {
    if (!memory_state.tracking_enabled || !ptr) return;

    AllocationRecord* record = malloc(sizeof(AllocationRecord));
    if (!record) {
        fprintf(stderr, "⚠️ Impossible de tracker l'allocation (out of memory)\n");
        return;
    }

    record->ptr = ptr;
    record->size = size;
    record->file = file;
    record->line = line;
    record->next = memory_state.allocations;

    memory_state.allocations = record;
    memory_state.allocation_count++;
    memory_state.total_allocated += size;

    size_t current_allocated = memory_state.total_allocated - memory_state.total_freed;
    if (current_allocated > memory_state.peak_memory) {
        memory_state.peak_memory = current_allocated;
    }
}

static void track_deallocation(void* ptr) {
    if (!memory_state.tracking_enabled || !ptr) return;

    AllocationRecord** current = &memory_state.allocations;
    while (*current) {
        if ((*current)->ptr == ptr) {
            AllocationRecord* to_remove = *current;
            *current = to_remove->next;

            memory_state.allocation_count--;
            memory_state.total_freed += to_remove->size;

            free(to_remove);
            return;
        }
        current = &((*current)->next);
    }

    fprintf(stderr, "⚠️ Tentative de libération d'un pointeur non tracké: %p\n", ptr);
}

void* safe_malloc_impl(size_t size, const char* file, int line) {
    if (size == 0) {
        debug_printf("⚠️ malloc(0) appelé depuis %s:%d\n", file, line);
        return NULL;
    }

    void* ptr = malloc(size);

    if (!ptr) {
        debug_printf("❌ ALLOCATION ÉCHOUÉE: %zu octets depuis %s:%d\n",
                    size, file, line);
        return NULL;
    }

    if (memory_state.tracking_enabled) {
        track_allocation(ptr, size, file, line);
    }

    return ptr;
}

void safe_free_impl(void** ptr, const char* file, int line) {
    (void)file;  // Supprime warning unused parameter
    (void)line;  // Supprime warning unused parameter

    if (!ptr || !*ptr) {
        return;
    }

    if (memory_state.tracking_enabled) {
        track_deallocation(*ptr);
    }

    free(*ptr);
    *ptr = NULL;
}

void memory_report_leaks(void) {
    if (!memory_state.tracking_enabled) {
        debug_printf("⚠️ Tracking désactivé, impossible de détecter les fuites\n");
        return;
    }

    debug_section("RAPPORT MÉMOIRE");

    debug_printf("📊 Statistiques globales:\n");
    debug_printf("  Total alloué  : %zu octets\n", memory_state.total_allocated);
    debug_printf("  Total libéré  : %zu octets\n", memory_state.total_freed);
    debug_printf("  Pic mémoire   : %zu octets\n", memory_state.peak_memory);
    debug_printf("  Allocations actives: %zu\n", memory_state.allocation_count);

    if (memory_state.allocation_count == 0) {
        debug_printf("✅ Aucune fuite mémoire détectée!\n");
        return;
    }

    debug_printf("\n❌ FUITES MÉMOIRE DÉTECTÉES (%zu blocs):\n",
                memory_state.allocation_count);
    debug_blank_line();

    AllocationRecord* current = memory_state.allocations;
    size_t leak_number = 1;
    size_t total_leaked = 0;

    while (current) {
        debug_printf("  Fuite #%zu:\n", leak_number++);
        debug_printf("    Adresse : %p\n", current->ptr);
        debug_printf("    Taille  : %zu octets\n", current->size);
        debug_printf("    Fichier : %s:%d\n", current->file, current->line);
        debug_blank_line();

        total_leaked += current->size;
        current = current->next;
    }

    debug_printf("💧 Total fuites: %zu octets non libérés\n", total_leaked);
}

size_t memory_get_allocation_count(void) {
    return memory_state.allocation_count;
}

size_t memory_get_allocated_bytes(void) {
    return memory_state.total_allocated - memory_state.total_freed;
}

void memory_cleanup_tracking(void) {
    if (!memory_state.tracking_enabled) return;

    AllocationRecord* current = memory_state.allocations;
    while (current) {
        AllocationRecord* next = current->next;
        free(current);
        current = next;
    }

    memory_state.allocations = NULL;
    memory_state.allocation_count = 0;

    debug_printf("🧹 Système de tracking mémoire nettoyé\n");
}
