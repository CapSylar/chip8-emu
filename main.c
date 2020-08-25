
#include "interpreter.c"
#include "audioqueue.c"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <sys/time.h>


#define instr_per_frame 9
#define MIN(a,b) (((a)<(b))?(a):(b)) // should not use this macro but whatever
#define MAX(a,b) (((a)<(b))?(b):(a))

void extract_pixels ( uint32_t *pixels  ) ;
void call_back ( void *userdata , uint8_t *stream , int len ) ;

audio_queue aq = { NULL } ;


int main ( int argc , char **argv )
{
    uint32_t pixels [W*H] ;
    SDL_Event event ;
    bool halt = false ;
    double duration ;
    int frames_done = 0  , frames = 0   ;

        if ( argc < 2 )
    {
        fprintf(stderr, "Please enter binary file name\n");
        exit(1) ;
    }

    FILE *program_fp = fopen ( *++argv , "rb" ) ;

    if ( program_fp == NULL )
    {
        fprintf(stderr, "ERROR: cannot open file for reading \n");
        exit(1) ;
    }

    init ( program_fp ) ; // load file into memory , install character fonts and setup registers


    // create window and initialise graphics

    if ( SDL_Init ( SDL_INIT_AUDIO ) != 0 )
    {
        fprintf(stderr, "%s\n", SDL_GetError() );
        return 1 ;
    }

    SDL_AudioSpec spec , obtained ;

    spec.freq = 44100 ;
    spec.format = AUDIO_S16SYS ;
    spec.channels = 2 ;
    spec.samples = spec.freq / 20 ;
    spec.callback = call_back ;
    SDL_OpenAudio ( &spec , &obtained ) ;
    SDL_PauseAudio(0) ;

    SDL_Window *window = SDL_CreateWindow ( *argv , SDL_WINDOWPOS_UNDEFINED , SDL_WINDOWPOS_UNDEFINED , W*16 , H*24 ,
        SDL_WINDOW_RESIZABLE ) ;
    SDL_Renderer *renderer = SDL_CreateRenderer ( window , -1 , 0 ) ;
    SDL_Texture *texture = SDL_CreateTexture ( renderer , SDL_PIXELFORMAT_ARGB8888 , SDL_TEXTUREACCESS_STREAMING , W , H  ) ;


    struct timeval start , current ;
    gettimeofday(&start, NULL);

    while ( !halt )
    {
        // run the cpu at 540Hz , 8 times the speed of the timers
        for ( int i = 0 ; i < instr_per_frame && !(waiting_key & 0x80 ); i++ )
            execute_cycle () ;

    // detect and map keys

    while ( SDL_PollEvent ( &event ) )
        switch ( event.type )
        {
            case SDL_QUIT: // handles x button on the upper right of the window
                halt = true ;
                break ;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                ; // put there because the C standart does not allow for declarations after labels only statements
                bool state = ( event.type == SDL_KEYDOWN ) ;
                uint8_t mapped_key ;
                switch ( event.key.keysym.scancode )
                {
                    case SDL_SCANCODE_ESCAPE :
                        halt = true ;
                        break ;
                    case SDL_SCANCODE_1 :
                        mapped_key = 0 ;
                        break ;
                    case SDL_SCANCODE_2 :
                        mapped_key = 1 ;
                        break ;
                    case SDL_SCANCODE_3 :
                        mapped_key = 2 ;
                        break ;
                    case SDL_SCANCODE_4 :
                        mapped_key = 3 ;
                        break ;
                    case SDL_SCANCODE_Q :
                        mapped_key = 4 ;
                        break ;
                    case SDL_SCANCODE_W :
                        mapped_key = 5 ;
                        break ;
                    case SDL_SCANCODE_E :
                        mapped_key = 6 ;
                        break ;
                    case SDL_SCANCODE_R :
                        mapped_key = 7 ;
                        break ;
                    case SDL_SCANCODE_A :
                        mapped_key = 8 ;
                        break ;
                    case SDL_SCANCODE_S :
                        mapped_key = 9 ;
                        break ;
                    case SDL_SCANCODE_D :
                        mapped_key = 0xA ;
                        break ;
                    case SDL_SCANCODE_F :
                        mapped_key = 0xB ;
                        break ;
                    case SDL_SCANCODE_Z :
                        mapped_key = 0xC ;
                        break ;
                    case SDL_SCANCODE_X :
                        mapped_key = 0xD ;
                        break ;
                    case SDL_SCANCODE_C :
                        mapped_key = 0xE ;
                        break ;
                    case SDL_SCANCODE_V :
                        mapped_key = 0xF ;
                        break ;
                }

            keys[mapped_key] = state ;
            if ( event.type == SDL_KEYDOWN && (waiting_key & 0x80) )
            {
                waiting_key &= 0x7F ; // clear waiting flag
                V[waiting_key] = mapped_key ;
            }

        }

    // calculate how many frames we had to render so far
    gettimeofday ( &current , NULL ) ;
    duration = ( double ) ( current.tv_usec - start.tv_usec ) / 1000000 + (double)(current.tv_sec - start.tv_sec);
    frames = ( int ) ( duration * 60 ) - frames_done ; // refreshing at 60 Hz for the display

    if ( frames > 0 )
    {
            // update timers
        frames_done += frames ;
        int st = MIN(frames,sound_timer) ; // does not allow decrementing past 0
        sound_timer -= st ;
        int dt = MIN(frames,delay_timer) ;
        delay_timer -= dt ;

        // render audio

        SDL_LockAudio() ;
        insert ( obtained.freq*st/60 , true , &aq ) ;
        insert ( obtained.freq*( frames - st )/60 , false , &aq ) ;
        SDL_UnlockAudio() ;



        // render graphics
        extract_pixels ( pixels ) ;
        SDL_UpdateTexture( texture , NULL , pixels , W*4 ) ; // each pixel is 4 bytes
        SDL_RenderCopy ( renderer , texture , NULL , NULL ) ;
        SDL_RenderPresent ( renderer ) ;


    }
        SDL_Delay ( 1000/60 ) ; // 60 Hz

    }

    fclose ( program_fp ) ;
    SDL_Quit();

}

void call_back ( void *userdata , uint8_t *stream , int len ) // callback function for SDL
{
    short *stream_s = ( short * ) stream ;

    while ( len > 0 && !is_empty ( &aq ) )
    {
        // peek into front object but do not remove it because we might not be able to
        // completely play it back

        node *data = peek ( &aq ) ;
        for (  ; len && data -> samples ;  len-=4 , stream_s+=2 , -- (data -> samples)  )
            stream_s[0] = stream_s[1] = data -> volume*300*((len&128)-64) ;
        if ( !data -> samples ) delete ( &aq ) ; // pop front because all samples are transferred

    }
}
