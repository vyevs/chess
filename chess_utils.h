#pragma once

// returns a string representing the move in algebraic notation
char *move_str(const struct move *move);

char *position_str(const struct position *position);

void load_fen_to_position(const char *fen, struct position *into);