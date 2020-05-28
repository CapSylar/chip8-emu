// interpreter part of the chip-8

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define H   32
#define W   64

void extract_pixels ( uint32_t *pixels  ) ;


uint8_t V[16] , sound_timer , delay_timer , sp , waiting_key ;

// waiting key is the variable of size 1 byte that will be used to indicate if the cpu is waiting for a key , if it is waiting
// the MSB will be on , the lower nibble of the byte will be used to indicate to which Vx we will write the keycode of the key

uint16_t i_register , pc , p  ;
uint8_t mem [4096] ;
uint16_t stack[16] ;
uint8_t display_mem [H*W/8] ;
bool keys [16] ;


uint8_t xor_test ( int a , uint8_t b )
{
    return ((display_mem[a] ^= b) ^ b) & b ;
}


void init ( FILE *program_fp )
{
    // load program into memory at 0x200
    int c ;

    for ( int i = 0x200 ; ( c = getc ( program_fp )) != EOF ; i++ )
        mem[i] = c ;

    // load character font into memory at locations 0x000 - 0x1FF

    uint8_t font[16*5] = { 0xF0 , 0x90 , 0x90 , 0x90 , 0xF0 , // 0
                           0x20 , 0x60 , 0x20 , 0x20 , 0x70 , // 1
                           0xF0 , 0x10 , 0xF0 , 0x80 , 0xF0 , // 2
                           0xF0 , 0x10 , 0xF0 , 0x10 , 0xF0 , // 3
                           0x90 , 0x90 , 0xF0 , 0x10 , 0x10 , // 4
                           0xF0 , 0x80 , 0xF0 , 0x10 , 0xF0 , // 5
                           0xF0 , 0x80 , 0xF0 , 0x90 , 0xF0 , // 6
                           0xF0 , 0x10 , 0x20 , 0x40 , 0x40 , // 7
                           0xF0 , 0x90 , 0xF0 , 0x90 , 0xF0 , // 8
                           0xF0 , 0x90 , 0xF0 , 0x10 , 0xF0 , // 9
                           0xF0 , 0x90 , 0xF0 , 0x90 , 0x90 , // A
                           0xE0 , 0x90 , 0xE0 , 0x90 , 0xE0 , // B
                           0xF0 , 0x80 , 0x80 , 0x80 , 0xF0 , // C
                           0xE0 , 0x90 , 0x90 , 0x90 , 0xE0 , // D
                           0xF0 , 0x80 , 0xF0 , 0x80 , 0xF0 , // E
                           0xF0 , 0x80 , 0xF0 , 0x80 , 0x80 } ; // F

    for ( int i = 0 ; i < 16*5 ; i++ )
        mem[i] = font[i] ;

    pc = 0x200 ; // set pc to start of program
    sp = -1 ;
}



void execute_cycle ( void )
{
    // at this point we already have the program loaded into memory at address 0x200
    // all registers are setup correctly

    uint16_t opcode = ( mem[pc] << 8 ) | mem[pc+1] ;
    pc += 2 ;

    uint16_t nnn = opcode & 0x0FFF ;
    uint8_t kk = opcode & 0x00FF ;
    uint8_t n = opcode & 0x000F , x = ( opcode & 0x0F00 ) >> 8 , y = ( opcode & 0x00F0 ) >> 4  ;





    switch ( opcode & 0xF000 ) // the first byte is the indicator for the type of opcode
    {
        case 0 :
            switch ( opcode )
            {
                case 0x00E0 : // clear the display
                    for ( int i = 0 ; i < H*W/8 ; i++ )
                        display_mem[i] = 0 ;
                    break;

                case 0x00EE : // return from subroutine
                    pc = stack[sp--] ;
                    break ;


                default :  // jump to machine code at adress nnn
                    pc = nnn ;

            }
            break ;

        case 0x1000 : // jump to address
            pc = nnn ;
            break ;

        case 0x2000 : // call subroutine at address nnn
            stack[++sp] = pc ;
            pc = nnn ;
            break ;

        case 0x3000 : // skip next instruction if Vx = kk
            if ( V[x] == kk )
                pc += 2 ;
                break ;

        case 0x4000 : // skip next instruction if Vx != kk
            if ( V[x] != kk )
                pc += 2 ;
                break ;

        case 0x5000 : // skip next instruction if Vx = Vy
            if ( V[x] == V[y] )
                pc += 2 ;
                break ;

        case 0x6000 : // load kk into register Vx
            V[x] = kk ;
            break ;

        case 0x7000 : // add kk to Vx
            V[x] += kk ;
            break ;

        case 0x8000 :
            switch ( n )
            {
                case 0 : // store Vy in Vx
                    V[x] = V[y] ;
                    break ;

                case 1 : // OR Vx and Vy then store in Vx
                    V[x] |= V[y] ;
                    break ;

                case 2 : // AND Vx and Vy then store in Vx
                    V[x] &= V[y] ;
                    break ;

                case 3 : // XOR Vx and Vy then store in Vx
                    V[x] ^= V[y] ;
                    break ;

                case 4 : // ADD Vx and Vy then store in Vx with update flags
                    p = V[x] + V[y] ;
                    V[15] =  p >> 8  ;
                    V[x] = p ;
                    break ;

                case 5 : // SUB substruct Vy from Vx then store in Vx with updated flags
                    p = V[x] - V[y] ;
                    V[15] = !( p >> 8 ) ;
                    V[x] = p ;
                    break ;

                case 6 : // if LSB of Vx is 1 then Vf = 1 , otherwise 0 then Vx/2
                    V[15] = V[x] & 0x01 ;
                    V[x] >>= 1  ;
                    break ;

                case 7 : // SUB substract Vx from Vy then store the result in Vx with flags
                    p = V[y]-V[x] ;
                    V[15] = !( p >> 8 ) ;
                    V[x] = p ;
                    break ;

                case 0xE : // if MSB of Vx is 1 then Vf = 1 , otherwise 0 then Vx*2
                    V[15] = V[x] & 0x80 ;
                    V[x] <<= 1  ;
                    break ;
            }

            break ;


        case 0x9000 : // skip next instruction if Vx != Vy
            if ( V[x] != V[y] )
                pc += 2 ;
            break ;

        case 0xA000 : // the value of register I is set to nnn
            i_register = nnn ;
            break ;

        case 0xB000 : // jump to location nnn + V0
            pc = nnn + V[0] ;
            break ;

        case 0xC000 : // generate random byte and AND with kk then store in Vx
            V[x] = ( rand() % 256 ) & kk ;
            break ;

        case 0xD000 : // display n-byte sprite at mem loc I at ( Vx , Vy ) , Vf = collision
            for ( kk = 0 ; n-- ; )
            {
                kk |= xor_test((V[x]%W+(V[y]+n)%H * W) / 8 , mem [ (i_register + n) & 0x0FFF ] >> (V[x]%8) )
                | xor_test ( ((V[x] + 7)%W + (V[y]+n)%H * W) / 8 , mem [ (i_register + n) & 0xFFF ] << ( 8 - V[x] % 8 )) ;
            }
            V[15] = ( kk == 0 ) ? 0 : 1 ;
            break ;

        case 0xE000 :
            switch ( n )
            {
                case 0xE : // skip next instruction if the key with the value of Vx is pressed
                    if ( keys[V[x]] == true )
                        pc += 2 ;
                    break ;

                case 1 : // skip next instruction if the key with value of Vx is not pressed
                    if ( keys[V[x]] == false )
                        pc += 2 ;
                    break ;
            }

            break ;


        case 0xF000 :
            switch ( kk )
            {
                case 0x07 : // load Vx with the delay timer
                    V[x] = delay_timer ;
                    break ;

                case 0x0A : // wait for the press of a button
                    waiting_key = 0x80 | x ; // signal which register to write into and the main function will take care of the rest
                    break ;

                case 0x15 : // set delay timer to Vx
                    delay_timer = V[x] ;
                    break ;

                case 0x18 : // set sound timer to Vx
                    sound_timer = V[x] ;
                    break ;

                case 0x1E : // add Vx to I and store back into I and update Vf = overflow
                    p = (i_register&0x0FFF) + V[x] ;
                    V[15] = p >> 12 ; // 12 because i is considered as a 12 bit register even if it is 16 bits
                    i_register = p ;

                    break ;

                case 0x29 : // set i to location of sprite for digit Vx
                    i_register = V[x] * 5 ; // since each sprite is 5 bytes long
                    break ;

                case 0x33 : // store bcd of Vx in memory location I , I+1 , I+2
                    mem[i_register & 0xFFF ] = ( V[x] / 100 ) ;
                    mem[(i_register+1) & 0xFFF ] = ( ( V[x] / 10 ) % 10  ) ;
                    mem[(i_register+2) & 0xFFF ] = ( V[x] % 10 ) ;

                    break ;

                case 0x55 : // store registers V0 through Vx in mem location I
                    for ( int i = 0 ; i <= x ; i++ )
                        mem[i_register++ & 0x0FFF ] = V[i] ;
                    break ;

                case 0x65 : // read registers V0 through Vx in mem location I
                    for ( int i = 0 ; i <= x ; i++ )
                        V[i] = mem[i_register++ & 0x0FFF ] ;
                    break ;

            }

            break ;


  }



}

void extract_pixels ( uint32_t *pixels  )
{
    for ( int i = 0 ; i < W*H ; i++ )
        pixels[i] = 0xFFFFFF * ( (display_mem[i/8] >> ( 7 - i%8 ) ) & 1  ) ;
}
