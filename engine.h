#pragma once

#include "chess.h"

void init_engine(void);

struct move find_best_move_for_color(struct position *the_position, bool is_piece_white);