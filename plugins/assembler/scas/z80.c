const char *z80_tab = "# z80 Instruction Table\n\
\n\
#### INSTRUCTION\n\
# INS [MNOMIC] [VALUE]\n\
# MNOMIC is any series of case-insenstive characters with support for special\n\
# characters to define additional functionality. MNOMIC may not have whitespace.\n\
# Special Characters:\n\
#  '_': Required whitespace\n\
#  '-': Optional whitespace\n\
#  '%#<bits[s]>': Immediate value (# is a character to use to identify later)\n\
#  '^#<bits[s]>': Immediate value relative to PC (# is a character to use to identify later)\n\
#  '@#<group>': Operand (# is a character to use to identify later)\n\
#  '&': Special - RST value\n\
# \n\
# VALUE is a value in binary, which may include references to immediate values and operands\n\
# in use above. For example, in the MNOMIC 'hello,-world_%A<16>_@B<a>', the value could be\n\
# '01011 %A 10110 @B'\n\
#\n\
#### OPERAND GROUP\n\
# OPERAND [GROUP NAME] [OPERAND NAME] [VALUE]\n\
# GROUP NAME is the name of the operand group this belongs to. This is used to reference the\n\
# group in a MNOMIC with the @ operator. OPERAND NAME is the name to match, such as A, B, IX,\n\
# etc. VALUE is the value in binary of this operand.\n\
\n\
# Z80 INSTRUCTION SET\n\
\n\
#### Z80 PROPERTIES\n\
ARCH z80\n\
\n\
#### OPERAND GROUPS\n\
# GROUP 1\n\
OPERAND g1 NZ 00\n\
OPERAND g1 Z  01\n\
OPERAND g1 NC 10\n\
OPERAND g1 C  11\n\
\n\
# GROUP 2\n\
OPERAND g2 NZ 000\n\
OPERAND g2 Z  001\n\
OPERAND g2 NC 010\n\
OPERAND g2 C  011\n\
OPERAND g2 PO 100\n\
OPERAND g2 PE 101\n\
OPERAND g2 P  110\n\
OPERAND g2 M  111\n\
\n\
# GROUP 3\n\
OPERAND g3 A    111\n\
OPERAND g3 B    000\n\
OPERAND g3 C    001\n\
OPERAND g3 D    010\n\
OPERAND g3 E    011\n\
OPERAND g3 H    100\n\
OPERAND g3 L    101\n\
OPERAND g3 (HL) 110\n\
\n\
# GROUP 4\n\
OPERAND g4 BC 0\n\
OPERAND g4 DE 1\n\
\n\
# GROUP 5\n\
OPERAND g5 I  0\n\
OPERAND g5 R  1\n\
\n\
# GROUP 6\n\
OPERAND g6 BC 00\n\
OPERAND g6 DE 01\n\
OPERAND g6 HL 10\n\
OPERAND g6 SP 11\n\
\n\
# GROUP 7\n\
OPERAND g7 BC 00\n\
OPERAND g7 DE 01\n\
OPERAND g7 HL 10\n\
OPERAND g7 AF 11\n\
\n\
# GROUP 8\n\
OPERAND g8 IX 11011101\n\
OPERAND g8 IY 11111101\n\
\n\
# GROUP 9\n\
OPERAND g9 BC 00\n\
OPERAND g9 DE 01\n\
OPERAND g9 IX 10\n\
OPERAND g9 SP 11\n\
\n\
# GROUP 10\n\
OPERAND g10 BC 00\n\
OPERAND g10 DE 01\n\
OPERAND g10 IY 10\n\
OPERAND g10 SP 11\n\
\n\
# Most instructions were just lifted from Learn TI-83+ Assembly in 28 Days, they've got\n\
# pretty nice instruction set documentation.\n\
#### INSTRUCTIONS\n\
\n\
# Undocumented IX/IY(H,L) instructions come first for parsing reasons\n\
\n\
# IXH\n\
INS ADD_A-,-IXH                         11011101 10000100\n\
    INS ADD_IXH                         11011101 10000100\n\
\n\
INS AND_IXH                             11011101 10100100\n\
    INS AND_A-,-IXH                     11011101 10100100\n\
\n\
INS CP_IXH                              11011101 10111100\n\
    INS A-,-CP_IXH                      11011101 10111100\n\
\n\
INS DEC_IXH                             11011101 00100101\n\
\n\
INS INC_IXH                             11011101 00100100\n\
\n\
INS LD_@A<g3>-,-IXH                     11011101 01@A100\n\
INS LD_IXH-,-@A<g3>                     11011101 01100@A\n\
INS LD_IXH-,-%A<8>                      11011101 00100110 %A\n\
\n\
INS OR_IXH                              11011101 10110100\n\
    INS OR_A-,-IXH                      11011101 10110100\n\
\n\
INS SBC_A-,-IXH                         11011101 10011100\n\
    INS SBC_IXH                         11011101 10011100\n\
\n\
INS SUB_IXH                             11011101 10010100\n\
    INS SUB_A-,-IXH                     11011101 10010100\n\
\n\
INS XOR_IXH                             11011101 10101100\n\
    INS XOR_A-,-IXH                     11011101 10101100\n\
\n\
# IXL\n\
INS ADD_A-,-IXL                         11011101 10000101\n\
    INS ADD_IXL                         11011101 10000101\n\
\n\
INS AND_IXL                             11011101 10100101\n\
    INS AND_A-,-IXL                     11011101 10100101\n\
\n\
INS CP_IXL                              11011101 10111101\n\
    INS A-,-CP_IXL                      11011101 10111101\n\
\n\
INS DEC_IXL                             11011101 00101101\n\
\n\
INS INC_IXL                             11011101 00101100\n\
\n\
INS LD_@A<g3>-,-IXL                     11011101 01@A101\n\
INS LD_IXL-,-@A<g3>                     11011101 01101@A\n\
INS LD_IXL-,-%A<8>                      11011101 00101110 %A\n\
\n\
INS OR_IXL                              11011101 10110101\n\
    INS OR_A-,-IXL                      11011101 10110101\n\
\n\
INS SBC_A-,-IXL                         11011101 10011101\n" \
"\
    INS SBC_IXL                         11011101 10011101\n\
\n\
INS SUB_IXL                             11011101 10010101\n\
    INS SUB_A-,-IXL                     11011101 10010101\n\
\n\
INS XOR_IXL                             11011101 10101101\n\
    INS XOR_A-,-IXL                     11011101 10101101\n\
\n\
# IYH\n\
INS ADD_A-,-IYH                         11111101 10000100\n\
    INS ADD_IYH                         11111101 10000100\n\
\n\
INS AND_IYH                             11111101 10100100\n\
    INS AND_A-,-IYH                     11111101 10100100\n\
\n\
INS CP_IYH                              11111101 10111100\n\
    INS A-,-CP_IYH                      11111101 10111100\n\
\n\
INS DEC_IYH                             11111101 00100101\n\
\n\
INS INC_IYH                             11111101 00100100\n\
\n\
INS LD_@A<g3>-,-IYH                     11111101 01@A100\n\
INS LD_IYH-,-@A<g3>                     11111101 01100@A\n\
INS LD_IYH-,-%A<8>                      11111101 00100110 %A\n\
\n\
INS OR_IYH                              11111101 10110100\n\
    INS OR_A-,-IYH                      11111101 10110100\n\
\n\
INS SBC_A-,-IYH                         11111101 10011100\n\
    INS SBC_IYH                         11111101 10011100\n\
\n\
INS SUB_IYH                             11111101 10010100\n\
    INS SUB_A-,-IYH                     11111101 10010100\n\
\n\
INS XOR_IYH                             11111101 10101100\n\
    INS XOR_A-,-IYH                     11111101 10101100\n\
\n\
# IYL\n\
INS ADD_A-,-IYL                         11111101 10000101\n\
    INS ADD_IYL                         11111101 10000101\n\
\n\
INS AND_IYL                             11111101 10100101\n\
    INS AND_A-,-IYL                     11111101 10100101\n\
\n\
INS CP_IYL                              11111101 10111101\n\
    INS A-,-CP_IYL                      11111101 10111101\n\
\n\
INS DEC_IYL                             11111101 00101101\n\
\n\
INS INC_IYL                             11111101 00101100\n\
\n\
INS LD_@A<g3>-,-IYL                     11111101 01@A101\n\
INS LD_IYL-,-@A<g3>                     11111101 01101@A\n\
INS LD_IYL-,-%A<8>                      11111101 00101110 %A\n\
\n\
INS OR_IYL                              11111101 10110101\n\
    INS OR_A-,-IYL                      11111101 10110101\n\
\n\
INS SBC_A-,-IYL                         11111101 10011101\n\
    INS SBC_IYL                         11111101 10011101\n\
\n\
INS SUB_IYL                             11111101 10010101\n\
    INS SUB_A-,-IYL                     11111101 10010101\n\
\n\
INS XOR_IYL                             11111101 10101101\n\
    INS XOR_A-,-IYL                     11111101 10101101\n\
\n\
#### DATA MOVEMENT\n\
\n\
INS EX_DE-,-HL                          11101011\n\
    INS EX_HL-,-DE                      11101011\n\
INS EX_AF-,-AF'                         00001000\n\
    INS EX_AF'-,-AF                     00001000\n\
INS EX_(-SP-)-,-HL                      11100011\n\
    INS EX_HL-,-(-SP-)                  11100011\n\
INS EX_(-SP-)-,-@A<g8>                  @A 11100011\n\
    INS EX_@A<g8>-,-(-SP-)              @A 11100011\n\
\n\
INS EXX                                 11011001\n\
\n\
INS LD_HL-,-(-%A<16>-)                  00101010 %A\n\
\n\
INS LD_@A<g3>-,-@B<g3>                  01@A@B\n\
\n\
INS LD_@A<g3>-,-(-@B<g8>-%C<8>-)        @B 01@A110 %C\n\
    INS LD_@A<g3>-,-(-%C<8>-+-@B<g8>-)  @B 01@A110 %C\n\
    INS LD_@A<g3>-,-(-@B<g8>-)          @B 01@A110 00000000\n\
INS LD_(-@A<g8>-%B<8>-)-,-@C<g3>        @A 01110@C %B\n\
    INS LD_(-%B<8>-+-@A<g8>-)-,-@C<g3>  @A 01110@C %B\n\
    INS LD_(-@A<g8>-)-,-@C<g3>          @A 01110@C 00000000\n\
\n\
INS LD_A-,-(-@A<g4>-)                   000@A1010\n\
INS LD_A-,-(-%A<16>-)                   00111010 %A\n\
INS LD_(-@A<g4>-)-,-A                   000@A0010\n\
INS LD_(-%A<16>-)-,-A                   00110010 %A\n\
INS LD_A-,-@A<g5>                       11101101 0101@A111\n\
INS LD_@A<g5>-,-A                       11101101 0100@A111\n\
\n\
INS LD_@A<g3>-,-%B<8>                   00@A110 %B\n\
\n\
INS LD_(@A<g8>-%B<8>-)-,-%C<8>          @A 00110110 %B %C\n\
    INS LD_(-@A<g8>-+-%B<8>-)-,-%C<8>   @A 00110110 %B %C\n\
    INS LD_(-%B<8>-+-@A<g8>-)-,-%C<8>   @A 00110110 %B %C\n\
    INS LD_(-@A<g8>-)-,-%B<8>           @A 00110110 00000000 %B\n\
\n\
INS LD_(-@A<g8>-%B<8>-)-,-%C<8>         @A 00110110 %B %C\n\
    INS LD_(-%B<8>-+-@A<g8>-)-,-%C<8>   @A 00110110 %B %C\n\
    INS LD_(-@A<g8>-)-,-%B<8>           @A 00110110 00000000 %B\n\
\n\
INS LD_SP-,-HL                          11111001\n\
INS LD_SP-,-@A<g8>                      @A 11111001\n\
\n\
INS LD_@A<g6>-,-(-%B<16>-)              11101101 01@A1011 %B\n\
INS LD_@A<g8>-,-(-%B<16>-)              @A 00101010 %B\n\
INS LD_@A<g6>-,-%B<16>                  00@A0001 %B\n\
INS LD_@A<g8>-,-%B<16>                  @A 00100001 %B\n\
INS LD_(-%A<16>-)-,-HL                  00100010 %A\n\
INS LD_(-%A<16>-)-,-@B<g6>              11101101 01@B0011 %A\n\
INS LD_(-%A<16>-)-,-@B<g8>              @B 00100010 %A\n\
INS LD_SP-,-HL                          11111001\n\
INS LD_SP-,-@A<g8>                      @A 11111001\n\
\n\
INS LDD                                 11101101 10101000\n\
\n\
INS LDDR                                11101101 10111000\n\
\n\
INS LDI                                 11101101 10100000\n\
\n\
INS LDIR                                11101101 10110000\n\
\n\
INS POP_@A<g7>                          11@A0001\n\
INS POP_@A<g8>                          @A 11100001\n\
\n\
INS PUSH_@A<g7>                         11@A0101\n\
INS PUSH_@A<g8>                         @A 11100101\n\
\n\
#### ARITHMETIC\n\
\n\
INS ADC_A-,-@A<g3>                      10001@A\n\
    INS ADC_@A<g3>-,-A                  10001@A\n\
    INS ADC_@A<g3>                      10001@A\n\
INS ADC_A-,-(-@A<g8>-%B<8>-)            @A 10001110 %B\n\
    INS ADC_A-,-(-%B<8>-+-@A<g8>-)      @A 10001110 %B\n\
    INS ADC_A-,-(-@A<g8>-)              @A 10001110 00000000\n\
INS ADC_HL-,-@A<g6>                     11101101 01@A1010\n\
INS ADC_A-,-%A<8>                       11001110 %A\n\
    INS ADC_%A<8>                       11001110 %A\n\
\n\
INS ADD_A-,-@A<g3>                      10000@A\n\
    INS ADD_-@A<g3>-,-A                 10000@A\n\
    INS ADD_@A<g3>                      10000@A\n\
INS ADD_A-,-(-@A<g8>-%B<8>-)            @A 10000110 %B\n\
    INS ADD_A-,-(-%B<8>-+-@A<g8>-)      @A 10000110 %B\n\
    INS ADD_A-,-(-@A<g8>-)              @A 10000110 00000000\n\
    INS ADD_-(-@A<g8>-+-%B<8>-)         @A 10000110 %B\n\
    INS ADD_-(-%B<8>-+-@A<g8>-)         @A 10000110 %B\n\
    INS ADD_-(-@A<g8>-)                 @A 10000110 00000000\n\
INS ADD_A-,-%A<8>                       11000110 %A\n\
INS ADD_HL-,-@A<g6>                     00@A1001\n\
INS ADD_IX-,-@A<g9>                     11011101 00@A1001\n\
INS ADD_IY-,-@A<g10>                    11111101 00@A1001\n\
\n\
INS CP_@A<g3>                           10111@A\n\
    INS CP_A-,-@A<g3>                   10111@A\n\
INS CP_(-@A<g8>-%B<8>-)                 @A 10111110 %B\n\
    INS CP_(-%B<8>-+-@A<g8>-)           @A 10111110 %B\n\
    INS CP_A-,-(-%B<8>-+-@A<g8>-)       @A 10111110 %B\n\
    INS CP_A-,-(-@A<g8>-+-%B<8>-)       @A 10111110 %B\n\
    INS CP_A-,-(-@A<g8>-)               @A 10111110 00000000\n\
    INS CP_(-@A<g8>-)                   @A 10111110 00000000\n\
INS CP_A-,-%A<8>                        11111110 %A\n\
    INS CP_%A<8>                        11111110 %A\n\
\n\
INS CPD                                 11101101 10101001\n\
\n\
INS CPDR                                11101101 10111001\n\
\n\
INS CPI                                 11101101 10100001\n\
\n\
INS CPIR                                11101101 10110001\n\
\n\
INS CPL                                 00101111\n\
\n\
INS DAA                                 00100111\n\
\n\
INS DEC_@A<g3>                          00@A101\n\
INS DEC_(-@A<g8>-%B<8>-)                @A 00110101 %B\n\
    INS DEC_(-%B<8>-+-@A<g8>-)          @A 00110101 %B\n\
    INS DEC_(-@A<g8>-)                  @A 00110101 00000000\n\
INS DEC_@A<g6>                          00@A1011\n\
INS DEC_@A<g8>                          @A 00101011\n\
\n\
INS INC_@A<g3>                          00@A100\n\
INS INC_(-@A<g8>-%B<8>-)                @A 00110100 %B\n\
    INS INC_(-%B<8>-+-@A<g8>-)          @A 00110100 %B\n\
    INS INC_(-@A<g8>-)                  @A 00110100 00000000\n\
INS INC_@A<g6>                          00@A0011\n\
INS INC_@A<g8>                          @A 00100011\n\
\n\
INS NEG                                 11101101 01000100\n\
\n\
INS SBC_HL-,-@A<g6>                     11101101 01@A0010\n\
INS SBC_A-,-@A<g3>                      10011@A\n\
    INS SBC_@A<g3>                      10011@A\n\
INS SBC_A-,-(-@A<g8>-%B<8>-)            @A 10011110 %B\n\
    INS SBC_A-,-(-%B<8>-+-@A<g8>-)      @A 10011110 %B\n\
    INS SBC_A-,-(-@A<g8>-)              @A 10011110 00000000\n\
INS SBC_A-,-%A<8>                       11011110 %A\n\
    INS SBC_%A<8>                       11011110 %A\n\
\n\
INS SUB_A-,-@A<g3>                      10010@A\n\
    INS SUB_@A<g3>                      10010@A\n\
INS SUB_A-,-(-@A<g8>-%B<8>-)            @A 10010110 %B\n\
    INS SUB_A-,-(-%B<8>-+-@A<g8>-)      @A 10010110 %B\n\
    INS SUB_A-,-(-@A<g8>-)              @A 10010110 00000000\n\
    INS SUB_(-@A<g8>-%B<8>-)            @A 10010110 %B\n\
    INS SUB_(-%B<8>-+-@A<g8>-)          @A 10010110 %B\n\
    INS SUB_(-@A<g8>-)                  @A 10010110 00000000\n\
INS SUB_A-,-%A<8>                       11010110 %A\n\
    INS SUB_%A<8>                       11010110 %A\n\
\n\
#### BIT MANIPULATION\n\
\n\
INS AND_@A<g3>                          10100@A\n\
    INS AND_A-,-@A<g3>                  10100@A\n\
INS AND_(-@A<g8>-%B<8>-)                @A 10100110 %B\n\
    INS AND_(-%B<8>-+-@A<g8>-)          @A 10100110 %B\n\
    INS AND_(-@A<g8>-)                  @A 10100110 00000000 00000000\n\
    INS AND_A-,-(-@A<g8>-%B<8>-)        @A 10100110 %B\n\
    INS AND_A-,-(-%B<8>-+-@A<g8>-)      @A 10100110 %B\n\
INS AND_A-,-%A<8>                       11100110 %A\n\
    INS AND_%A<8>                       11100110 %A\n\
\n\
INS BIT_%A<3>-,-@B<g3>                  11001011 01%A@B\n\
INS BIT_%A<3>-,-(-@B<g8>-%C<8>-)        @B 11001011 %C 01%A110\n\
    INS BIT_%A<3>-,-(-%C<8>-+-@B<g8>-)  @B 11001011 %C 01%A110\n\
    INS BIT_%A<3>-,-(-@B<g8>-)          @B 11001011 00000000 01%A110\n\
\n\
INS CCF 00111111\n\
\n\
INS OR_@A<g3>                           10110@A\n\
    INS OR_A-,-@A<g3>                   10110@A\n\
INS OR_(-@A<g8>-%B<8>-)                 @A 10110110 %B\n\
    INS OR_(-%B<8>-+-@A<g8>-)           @A 10110110 %B\n\
    INS OR_(-@A<g8>-)                   @A 10110110 00000000\n\
    INS OR_A-,-(-@A<g8>-%B<8>-)         @A 10110110 %B\n\
    INS OR_A-,-(-%B<8>-+-@A<g8>-)       @A 10110110 %B\n\
    INS OR_A-,-(-@A<g8>-)               @A 10110110 00000000\n\
INS OR_A-,-%A<8>                        11110110 %A\n\
    INS OR_%A<8>                        11110110 %A\n\
\n\
INS RES_%A<3>-,-@B<g3>                  11001011 10%A@B\n\
INS RES_%A<3>-,-(-@B<g8>-%C<8>-)        @B 11001011 %C 10%A110\n\
    INS RES_%A<3>-,-(-%C<8>-+-@B<g8>-)  @B 11001011 %C 10%A110\n\
    INS RES_%A<3>-,-(-@B<g8>-)          @B 11001011 00000000 10%A110\n\
\n\
INS SCF 00110111\n\
\n\
INS SET_%A<3>-,-@B<g3>                  11001011 11%A@B\n\
INS SET_%A<3>-,-(-@B<g8>-%C<8>-)        @B 11001011 %C 11%A110\n\
    INS SET_%A<3>-,-(-%C<8>-+-@B<g8>-)  @B 11001011 %C 11%A110\n\
    INS SET_%A<3>-,-(-@B<g8>-)          @B 11001011 00000000 11%A110\n\
\n\
INS XOR_@A<g3>                          10101@A\n\
    INS XOR_A-,-@A<g3>                  10101@A\n\
INS XOR_(-@A<g8>-%B<8>-)                @A 10101110 %B\n\
    INS XOR_(-%B<8>-+-@A<g8>-)          @A 10101110 %B\n\
    INS XOR_(-@A<g8>-)                  @A 10101110 00000000\n\
    INS XOR_A-,-(-@A<g8>-%B<8>-)        @A 10101110 %B\n\
    INS XOR_A-,-(-%B<8>-+-@A<g8>-)      @A 10101110 %B\n\
    INS XOR_A-,-(-@A<g8>-)              @A 10101110 00000000\n\
INS XOR_A-,-%A<8>                       11101110 %A\n\
    INS XOR_%A<8>                       11101110 %A\n\
\n\
#### SHIFT/ROTATE\n\
\n\
INS RL_@A<g3>                           11001011 00010@A\n\
INS RL_(-@A<g8>-%B<8>-)                 @A 11001011 %B 00010110\n\
    INS RL_(-%B<8>-+-@A<g8>-)           @A 11001011 %B 00010110\n\
    INS RL_(-@A<g8>-)                   @A 11001011 00000000 00010110\n\
\n\
INS RLA 00010111\n\
\n\
INS RLC_@A<g3>                          11001011 00000@A\n\
INS RLC_(-@A<g8>-%B<8>-)                @A 11001011 %B 00000110\n\
    INS RLC_(-%B<8>-+-@A<g8>-)          @A 11001011 %B 00000110\n\
    INS RLC_(-@A<g8>-)                  @A 11001011 00000000 00000110\n\
\n\
INS RLCA                                00000111\n\
\n\
INS RLD                                 11101101 01101111\n\
\n\
INS RR_@A<g3>                           11001011 00011@A\n\
\n\
INS RR_(-@A<g8>-%B<8>-)                 @A 11001011 %B 00011110\n\
    INS RR_(-%B<8>-+-@A<g8>-)           @A 11001011 %B 00011110\n\
    INS RR_(-@A<g8>-)                   @A 11001011 00000000 00011110\n\
\n\
INS RRA                                 00011111\n\
\n\
INS RRC_@A<g3>                          11001011 00001@A\n\
INS RRC_(-@A<g8>-%B<8>-)                @A 11001011 %B 00001110\n\
    INS RRC_(-%B<8>-+-@A<g8>-)          @A 11001011 %B 00001110\n\
    INS RRC_(-@A<g8>-)                  @A 11001011 00000000 00001110\n\
\n\
INS RRCA                                00001111\n\
\n\
INS RRD                                 11101101 01100111\n\
\n\
INS SLA_@A<g3>                          11001011 00100@A\n\
INS SLA_(-@A<g8>-%B<8>-)                @A 11001011 %B 00100110\n\
    INS SLA_(-%B<8>-+-@A<g8>-)          @A 11001011 %B 00100110\n\
    INS SLA_(-@A<g8>-)                  @A 11001011 00000000 00100110\n\
\n\
INS SRA_@A<g3>                          11001011 00101@A\n\
INS SRA_(-@A<g8>-%B<8>-)                @A 11001011 %B 00101110\n\
    INS SRA_(-%B<8>-+-@A<g8>-)          @A 11001011 %B 00101110\n\
    INS SRA_(-@A<g8>-)                  @A 11001011 00000000 00101110\n\
\n\
INS SRL_@A<g3>                          11001011 00111@A\n\
INS SRL_(-@A<g8>-%B<8>-)                @A 11001011 %B 00111110\n\
    INS SRL_(-%B<8>-+-@A<g3>-)          @A 11001011 %B 00111110\n\
    INS SRL_(-@A<g3>-)                  @A 11001011 00000000 00111110\n\
\n\
#### CONTROL\n\
\n\
INS CALL_@A<g2>-,-%B<16>                11@A100 %B\n\
INS CALL_%A<16>                         11001101 %A\n\
\n\
INS DJNZ_^A<8>                          00010000 ^A\n\
\n\
INS JP_(-HL-)                           11101001\n\
INS JP_HL                               11101001\n\
INS JP_(-@A<g8>-)                       @A 11101001\n\
\n\
INS JP_@A<g2>-,-%B<16>                  11@A010 %B\n\
INS JP_%A<16>                           11000011 %A\n\
\n\
INS JR_@A<g1>-,-^B<8>                   001@A000 ^B\n\
INS JR_^A<8>                            00011000 ^A\n\
\n\
INS NOP                                 00000000\n\
\n\
INS RET                                 11001001\n\
INS RET_@A<g2>                          11@A000\n\
\n\
INS RETI                                11101101 01001101\n\
\n\
INS RETN                                11101101 01000101\n\
\n\
INS RST_&A                              11&A111\n\
\n\
#### HARDWARE\n\
\n\
INS DI                                  11110011\n\
\n\
INS EI                                  11111011\n\
\n\
INS HALT                                01110110\n\
\n\
INS IM_0                                11101101 01000110\n\
INS IM_1                                11101101 01010110\n\
INS IM_2                                11101101 01011110\n\
\n\
INS IN_@A<g3>-,-(-C-)                   11101101 01@A000\n\
INS IN_A-,-(-%A<8>-)                    11011011 %A\n\
\n\
INS IND                                 11101101 10101010\n\
\n\
INS INDR                                11101101 10111010\n\
\n\
INS INI                                 11101101 10100010\n\
\n\
INS INIR                                11101101 10110010\n\
\n\
INS OTDR                                11101101 10111011\n\
\n\
INS OTIR                                11101101 10110011\n\
\n\
INS OUT_(-C-)-,-@A<g3>                  11101101 01@A001\n\
INS OUT_(-%A<8>-)-,-A                   11010011 %A\n\
\n\
INS OUTD                                11101101 10101011\n\
\n\
INS OUTI                                11101101 10100011\n\
\n\
# HARDWARE\n\
INS IN_(-C-)                            11101101 01110000\n\
    INS IN_F-,-(-C-)                    11101101 01110000\n\
INS OUT_(-C-)-,-0                       11101101 01110001\n\
";
