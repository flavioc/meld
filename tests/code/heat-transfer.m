meld fil       d                                        
                                                                                           	              
                                                               (                                                                       )              2                                                        !              *              3              <                                                        "               +       !       4       "       =       #       F       $              %              &              '       #       (       ,       )       5       *       >       +       G       ,       P       -       	       .              /              0       $       1       -       2       6       3       ?       4       H       5       Q       6       Z       7              8              9       %       :       .       ;       7       <       @       =       I       >       R       ?       [       @              A       &       B       /       C       8       D       A       E       J       F       S       G       \       H       '       I       0       J       9       K       B       L       K       M       T       N       ]       O       1       P       :       Q       C       R       L       S       U       T       ^       U       ;       V       D       W       M       X       V       Y       _       Z       E       [       N       \       W       ]       `       ^       O       _       X       `       a       a       Y       b       b       c       c       	                _init -o node-axioms.i   neighborchanged(B, V, O), delta(P1, P2), !edge(MV1, W), MV1 = B -o 
			delta(P1 + (V * W), P2 + (O * W)).J   -o (heatchanged(O, X), heat(V), !inbound(T), fabs(X) > 7.5 -o 
			heat(V), {B, W | !edge(B, W) | add-priority(fabs(X) / 7.5)@B, 
			neighborchanged(host-id, V / float(T), O / float(T))@B}), OR (heatchanged(O, X), heat(V), !inbound(T) -o heat(V), 
			{B, W | !edge(B, W) | neighborchanged(host-id, V / float(T), O / float(T))@B}).�   delta(Plus, Minus), heat(V), count(C), !inbound(X), 
			fabs((Plus - Minus)) > 0.1 -o heatchanged(V, Plus - Minus), heat((V + Plus) - Minus), count(C + 1), delta(0.0, 0.0).           �       A   .        O �    1   .      �?H 5 �   .      �H 5 2!               _init                                                                setcolor                                                             setedgelabel                                                         write-string                                                         setcolor2                                                             stop-program                                                         set-priority                                                         add-priority                                                          schedule-next                                                        set-default-priority                                                  set-moving                                                            set-static                                                           set-affinity                                                         set-cpu                                                               remove-priority                                                      heatchanged                                                          neighborchanged                                                      delta                                                                count                                                                 inbound                                                               edge                                                                  coord                                                                  inner                                                                heat                                                             ��������   �&                   �&   � 
i&  d      D   '  �   {  �  �   �  K  �
  @  E  �    �  �  �    �  �  i  �    z  �  P  �  ;  <  t  �  J  �  "  �    �  �  D	  �    �  �  _  �  �  >  �	    �  �  Y  �  /  �!  �  
  y  �  R  �  )  �  �!  s
  �  K  �  #  �  �  B"  C  �    �  �  ^  �"    �  �  X  �  �"  �  R  �  (   >#  �  "  �   �#  �  �   �#  W!  :$  �$  >                    @          ����?          ����?�$  O                   @           ����?          ����?          ����?>$  O                   @          ����?          ����?          ����?�#  O                   @          ����?          ����?
          ����?�#  O                   @          ����?          ����?          ����?B#  O                   @
          ����?          ����?          ����?�"  O                   @          ����?          ����?          ����?�"  O                   @          ����?%          ����?$          ����?F"  O                   @          ����?.          ����?-          ����?�!  >          	         @$          ����?7          ����?�!  O                   @           ����?          ����?          ����?[!  `                  @          ����?          ����?          ����?          ����?�   `                  @          ����?          ����?          ����?          ����?�   `                  @          ����?          ����?          ����?          ����?,   `                  @
          ����?          ����?          ����?          ����?�  `                  @          ����?          ����?          ����?          ����?b  `                  @          ����?          ����?&          ����?%          ����?�  `                  @          ����?          ����?/          ����?.          ����?�  `                  @$          ����?%          ����?8          ����?7          ����?3  O         	         @-          ����?.          ����?@          ����?�  O                   @          ����?	          ����?          ����?�  `                  @          ����?          ����?          ����?          ����?&  `                  @          ����?          ����?          ����?          ����?�  `                  @          ����?          ����?          ����?          ����?\  `                  @          ����?          ����?          ����?          ����?�  `                  @          ����?          ����?'          ����?&          ����?�  `                  @          ����?          ����?0          ����?/          ����?-  `                  @%          ����?&          ����?9          ����?8          ����?�  `                  @.          ����?/          ����?A          ����?@          ����?c  O         	         @7          ����?8          ����?H          ����?  O                   @          ����?          ����?          ����?�  `                  @          ����?	          ����?          ����?          ����?V  `                  @          ����?          ����?          ����?          ����?�  `                  @          ����?          ����?           ����?          ����?�  `                  @          ����?          ����?(             �?'          ����?'  `                  @          ����?          ����?1             �?0          ����?�  `                  @&          ����?'          ����?:          ����?9          ����?]  `                  @/          ����?0          ����?B          ����?A          ����?�  `                  @8          ����?9          ����?I          ����?H          ����?�  O         	         @@          ����?A          ����?O          ����??  O                   @	          ����?          ����?          ����?�  `                  @          ����?          ����?          ����?          ����?�  `                  @          ����?          ����?!          ����?           ����?!  `                  @          ����?          ����?)          ����?(             �?�  a                  Y@             �?              �?2             �?1             �?V  a                  Y@'             �?(             �?;             �?:             �?�  `                  @0          ����?1             �?C          ����?B          ����?�  `                  @9          ����?:          ����?J          ����?I          ����?&  `                  @A          ����?B          ����?P          ����?O          ����?�  O         	         @H          ����?I          ����?U          ����?m  O                   @          ����?          ����?          ����?  `                  @          ����?          ����?"          ����?!          ����?�  `                  @          ����?          ����?*          ����?)          ����?O  `                  @           ����?!          ����?3          ����?2             �?�  a                  Y@(             �?)             �?<             �?;             �?�  a                  Y@1             �?2             �?D             �?C             �?  `                  @:          ����?;             �?K          ����?J          ����?�  `                  @B          ����?C          ����?Q          ����?P          ����?T  `                  @I          ����?J          ����?V          ����?U          ����?�  O         	         @O          ����?P          ����?Z          ����?�  O                   @          ����?#          ����?"          ����?G  `                  @          ����?          ����?+          ����?*          ����?�  `                  @!          ����?"          ����?4          ����?3          ����?}  `                  @)          ����?*          ����?=          ����?<          ����?  `                  @2             �?3          ����?E          ����?D          ����?�  `                  @;             �?<          ����?L          ����?K          ����?N  `                  @C          ����?D          ����?R          ����?Q          ����?�  `                  @J          ����?K          ����?W          ����?V          ����?�  `                  @P          ����?Q          ����?[          ����?Z          ����?  O         	         @U          ����?V          ����?^          ����?�
  O                   @          ����?,          ����?+          ����?w
  `                  @"          ����?#          ����?5          ����?4          ����?
  `                  @*          ����?+          ����?>          ����?=          ����?�	  `                  @3          ����?4          ����?F          ����?E          ����?H	  `                  @<          ����?=          ����?M          ����?L          ����?�  `                  @D          ����?E          ����?S          ����?R          ����?~  `                  @K          ����?L          ����?X          ����?W          ����?  `                  @Q          ����?R          ����?\          ����?[          ����?�  `                  @V          ����?W          ����?_          ����?^          ����?O  O         	         @Z          ����?[          ����?a          ����?�  O                   @#          ����?6          ����?5          ����?�  `                  @+          ����?,          ����??          ����?>          ����?B  `                  @4          ����?5          ����?G          ����?F          ����?�  `                  @=          ����?>          ����?N          ����?M          ����?x  `                  @E          ����?F          ����?T          ����?S          ����?  `                  @L          ����?M          ����?Y          ����?X          ����?�  `                  @R          ����?S          ����?]          ����?\          ����?I  `                  @W          ����?X          ����?`          ����?_          ����?�  `                  @[          ����?\          ����?b          ����?a          ����?  O         	         @^          ����?_          ����?c          ����?+  >      	             @,          ����??          ����?�  O      	            @5          ����?6          ����?G          ����?�  O      	            @>          ����??          ����?N          ����?@  O      	            @F          ����?G          ����?T          ����?�  O      	            @M          ����?N          ����?Y          ����?�  O      	            @S          ����?T          ����?]          ����?D  O      	            @X          ����?Y          ����?`          ����?�   O      	            @\          ����?]          ����?b          ����?�   O      	            @_          ����?`          ����?c          ����?H   >      	   	         @a          ����?b          ����?   @ -          -         w @ -          -         w @       w #         � h  q&  `&  .&  &  &  �%  �%  �%  �%  u%  d%  2%  !%  %  �$  �$  �$  �$  y$  h$  6$  %$  $  �#  �#  �#  �#  }#  K#  :#  )#  �"  �"  �"  �"  �"  �"  p"  _"  -"  "  "  �!  �!  �!  �!  �!  c!  R!  A!  0!  �   �   �   �   �   �   w   f   4   #         �  �  �  {  j  Y  '      �  �  �  �  �  ]  L  ;  *  �  �  �  �  �  �  q  `  .      �  �  �  �  �  d  S  B  1  �  �  �  �  �  �  W  F  5  $  �  �  �  �  �  |  k  Z  (      �  �  �  �  �  ^  M  <  +  �  �  �  �  �  �  r  a  /      �  �  �  �  v  e  T  "       �  �  �  �  �  X  G  6  %  �  �  �  �  �  {  j  Y  '      �  �  �  �  �  ]  L  ;  	  �  �  �  �  �  �  P  ?  .    �  �  �  �  �  u  d  S       �  �  �  �  �  �  U  D  3  "  �  �  �  �  �  z  i  7  &    �  �  �  �  ~  m  \  K      �  �  �  �  �  �  O  >  -    �  �  �  �  �  t  c  R       �  �  �  �  �  g  V  E      �  �  �  �  �  {  I  8  '    �
  �
  �
  �
  
  n
  ]
  L
  
  	
  �	  �	  �	  �	  �	  �	  P	  ?	  .	  	  �  �  �  �  �  u  C  2  !    �  �  �  �  y  h  W  F      �  �  �  �  �  |  J  9  (    �  �  �  �  �  o  ^  M    
  �  �  �  �  s  b  0      �  �  �  �  w  f  4  #    �  �  �  �  {  j  8  '    �  �      �                  {               e                O      " " "HF& "" "HF&{� �                         �    "  3! 4 .      @N`�              �               �    {v              p    @( " " 	I&"  " 	I&" "  3! 4 .      @I" ��� �            �               �               k    {P              J    @( " " 	I&"  " 	I&" �� �        �                  �    "  " G 3! 4 .   ����?N`�              �               �               w    @!  "  " G&w" "  F" G& {"    =& {-          -         { �        