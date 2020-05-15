#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_timer.h"
#include "SDL_log.h"
#include "SDL_image.h"

#include "board.h"

const uint32_t R_MASK = 0x000000ff;
const uint32_t G_MASK = 0x0000ff00;
const uint32_t B_MASK = 0x00ff0000;
const uint32_t A_MASK = 0xff000000;


SDL_Texture *white_king_texture;
SDL_Texture *black_king_texture;
SDL_Texture *white_queen_texture;
SDL_Texture *black_queen_texture;
SDL_Texture *white_pawn_texture;
SDL_Texture *black_pawn_texture;
SDL_Texture *white_bishop_texture;
SDL_Texture *black_bishop_texture;
SDL_Texture *white_knight_texture;
SDL_Texture *black_knight_texture;
SDL_Texture *white_rook_texture;
SDL_Texture *black_rook_texture;

SDL_Texture *load_piece_texture(SDL_Renderer *the_renderer, const char *name) {
	char path[32];
	sprintf(path, "assets/%s.png", name);
	
	SDL_Surface *piece_surface = IMG_Load(path);
	if (piece_surface == NULL) {
		fprintf(stderr, "IMG_Load %s: %s\n", name, IMG_GetError());
		exit(1);
	}

	SDL_Texture *piece_texture = SDL_CreateTextureFromSurface(the_renderer, piece_surface);
	if (piece_texture == NULL) {
		fprintf(stderr, "SDL_CreateTextureFromSurface for %s: %s\n", name, SDL_GetError());
		exit(1);
	}

	return piece_texture;
}

void load_piece_textures(SDL_Renderer *the_renderer) {
	white_king_texture = load_piece_texture(the_renderer, "white_king");
	black_king_texture = load_piece_texture(the_renderer, "black_king");;
	white_queen_texture = load_piece_texture(the_renderer, "white_queen");;
	black_queen_texture = load_piece_texture(the_renderer, "black_queen");;
	white_pawn_texture = load_piece_texture(the_renderer, "white_pawn");;
	black_pawn_texture = load_piece_texture(the_renderer, "black_pawn");;
	white_bishop_texture = load_piece_texture(the_renderer, "white_bishop");;
	black_bishop_texture = load_piece_texture(the_renderer, "black_bishop");;
	white_knight_texture = load_piece_texture(the_renderer, "white_knight");;
	black_knight_texture = load_piece_texture(the_renderer, "black_knight");;
	white_rook_texture = load_piece_texture(the_renderer, "white_rook");;
	black_rook_texture = load_piece_texture(the_renderer, "black_rook");;
}


void print_error_and_exit(const char *error) {
	fprintf(stderr, "%s failed: %s\n", error, SDL_GetError());
	exit(1);
}

void render_chessboard_grid(SDL_Renderer *the_renderer) {
	#define white true
	bool square_color = white;
	SDL_Rect the_rect;
	for (int row = 0; row < 8; row++) {
		int pixel_top = row * 100;
		int pixel_left = 0;

		for (int col = 0; col < 8; col++) {
			if (square_color == white)
				SDL_SetRenderDrawColor(the_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
			else
				SDL_SetRenderDrawColor(the_renderer, 165, 42, 42, SDL_ALPHA_OPAQUE);
			square_color = !square_color;

			the_rect.x = pixel_left;
			the_rect.y = pixel_top;
			the_rect.w = 100;
			the_rect.h = 100;

			SDL_RenderFillRect(the_renderer, &the_rect);

			pixel_left += 100;
		}

		square_color = !square_color;
	}
}

void render_pieces(SDL_Renderer *the_renderer, struct board *board) {
	for (int rank = 7; rank >= 0; rank--) {
		for (int file = 0; file < 8; file++) {
			if (!board->squares[rank][file].has_piece)
				continue;

			int pixel_top = (7 - rank) * 100;
			int pixel_left = file * 100;
			
			SDL_Rect dest_rect;
			dest_rect.x = pixel_left;
			dest_rect.y = pixel_top;
			dest_rect.w = 100;
			dest_rect.h = 100;
			
			piece_type piece_type = board->squares[rank][file].piece_type;
			piece_color piece_color = board->squares[rank][file].piece_color;
			
			SDL_Texture *piece_texture;
			switch (piece_type) {
				case PIECE_TYPE_PAWN: {
					if (piece_color == PIECE_COLOR_WHITE)
						piece_texture = white_pawn_texture;
					else
						piece_texture = black_pawn_texture;
				};
				break;

				case PIECE_TYPE_KING: {
					if (piece_color == PIECE_COLOR_WHITE)
						piece_texture = white_king_texture;
					else
						piece_texture = black_king_texture;
				};
				break;
				
				case PIECE_TYPE_QUEEN: {
					if (piece_color == PIECE_COLOR_WHITE)
						piece_texture = white_queen_texture;
					else
						piece_texture = black_queen_texture;
				};
				break;
				
				case PIECE_TYPE_BISHOP: {
					if (piece_color == PIECE_COLOR_WHITE)
						piece_texture = white_bishop_texture;
					else
						piece_texture = black_bishop_texture;
				};
				break;
				
				case PIECE_TYPE_KNIGHT: {
					if (piece_color == PIECE_COLOR_WHITE)
						piece_texture = white_knight_texture;
					else
						piece_texture = black_knight_texture;
				};
				break;
				
				case PIECE_TYPE_ROOK: {
					if (piece_color == PIECE_COLOR_WHITE)
						piece_texture = white_rook_texture;
					else
						piece_texture = black_rook_texture;
				};
				break;
			}
			
			SDL_RenderCopy(the_renderer, piece_texture, NULL, &dest_rect);
		}
	}
}

int main(int argc, char *argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		print_error_and_exit("SDL_Init");
	} else {
		printf("SDL_Init done\n");
	}

	int initted_flags = IMG_Init(IMG_INIT_PNG);
	if ((initted_flags & IMG_INIT_PNG) == 0) {
		fprintf(stderr, "IMG_Init could not init PNG stuff: %s\n", IMG_GetError());
		exit(1);
	}

	SDL_Window *the_window = SDL_CreateWindow("The Window!",
                                              SDL_WINDOWPOS_CENTERED,
                                              SDL_WINDOWPOS_CENTERED,
                                              1000,
                                              800,
                                              SDL_WINDOW_SHOWN);
	if (!the_window) {
		print_error_and_exit("SDL_CreateWindow");
	} else {
		SDL_Log("Created the window\n");
	}
      
	SDL_Renderer *the_renderer = SDL_CreateRenderer(the_window, -1, 0);
	if (the_renderer == NULL)
		print_error_and_exit("SDL_CreateRenderer");
	else
		printf("Created the renderer\n");
	
	load_piece_textures(the_renderer);

	
	struct board the_board;
	char *starting_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	load_fen_to_board(starting_position, &the_board);

	printf("the board ascii:\n%s\n", board_str(&the_board));

	bool running = true;

    while (running) {
		SDL_SetRenderDrawColor(the_renderer, 100, 100, 100, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(the_renderer);
		render_chessboard_grid(the_renderer);
		render_pieces(the_renderer, &the_board);
		SDL_RenderPresent(the_renderer);


        SDL_Event event;
        int res = SDL_WaitEvent(&event);
        if (res != 1) {
            printf("SDL_WaitEvent error: %s\n", SDL_GetError());
            continue;
        }
        
        switch (event.type) {
			case SDL_QUIT:
				running = false;
			break;
        }
        
    }
    
    
    return EXIT_SUCCESS;
}