meld fil       d                                                                                                                                   	       	       
       
                                                                                                                                                                                                                                                                                                                             !       !       "       "       #       #       $       $       %       %       &       &       '       '       (       (       )       )       *       *       +       +       ,       ,       -       -       .       .       /       /       0       0       1       1       2       2       3       3       4       4       5       5       6       6       7       7       8       8       9       9       :       :       ;       ;       <       <       =       =       >       >       ?       ?       @       @       A       A       B       B       C       C       D       D       E       E       F       F       G       G       H       H       I       I       J       J       K       K       L       L       M       M       N       N       O       O       P       P       Q       Q       R       R       S       S       T       T       U       U       V       V       W       W       X       X       Y       Y       Z       Z       [       [       \       \       ]       ]       ^       ^       _       _       `       `       a       a       b       b       c       c                      _init -o node-axioms.j   start(), !heat(H), !inbound(T) -o wait(T), 
			{B, W | !edge(B, W) | initial-neighbor-heat(host-id, H)@B}.[   new-neighbor-heat(B, Heat), neighbor-heat(MV1, OldHeat), MV1 = B -o neighbor-heat(B, Heat).<  update-neighbors(Diff, New) -o {B, W | !edge(B, W) | 
			if (Diff <= str2float(@arg1)) && (cpu-id(host-id) = cpu-id(B)) then new-neighbor-heat(host-id, New)@B,  end, if Diff > str2float(@arg1) then new-neighbor-heat(host-id, New)@B, 
			update()@B,  end, if Diff > str2float(@arg2) then !add-priority(Diff)@B,  end}.Z   initial-neighbor-heat(B, Heat), wait(T), T > 0 -o wait(T - 1), 
			neighbor-heat(B, Heat).   wait(MV2), MV2 = 0 -o update().   !update(), update() -o 1.�   update(), !inbound(T), heat(OldHeat) -o [sum => X | B, NewHeat | , 
			!neighbor-heat(B, X) | NewHeat := (X + OldHeat) / float((T + 1)), heat(NewHeat), update-neighbors(fabs((NewHeat - OldHeat)), NewHeat)].          | i
  )     | i
  )    �                     _init                                                                  inbound                                                                 edge                                                                   coord                                                                   inner                                                                  heat                                                                    neighbor-heat                                                           new-neighbor-heat                                                       initial-neighbor-heat                                                   start                                                                  wait                                                                    update                                                                 update-neighbors                                                 �������   �&                   �&   � 
�&  d      F   �   �   E  �  �  D  �  �  3  �  �  R  �    �  �  K  �    Z  �  $  �  �  S	  �	  
  �
  �
  ,  �  �  [  �  %  �  �  T  �  �  c  �  -  �  �  \  �  &  {  �  5  �  �  d  �  .  �  �  M  �    l  �  6  �     e  �    t  �  >  �    m  �  7  �  �  F  �    u  �  ?   �   	!  n!  �!  "  ]"  �"  #  \#  �#  $  [$  �$  @                  @   
          ����?          ����?�$  P                  @              ����?          ����?          ����?_$  P                  @             ����?          ����?          ����?
$  P                  @             ����?          ����?          ����?�#  P                  @             ����?          ����?          ����?`#  P                  @             ����?          ����?          ����?#  P                  @             ����?          ����?          ����?�"  P                  @             ����?          ����?          ����?a"  P                  @             ����?          ����?	          ����?"  @                  @             ����?          ����?�!  P                  @              ����?          ����?          ����?r!  `                  @             ����?
          ����?          ����?          ����?!  `                  @             ����?          ����?          ����?          ����?�   `                  @             ����?          ����?          ����?          ����?C   `                  @             ����?          ����?          ����?          ����?�  `                  @             ����?          ����?          ����?          ����?y  `                  @             ����?          ����?          ����?          ����?  `                  @             ����?          ����?          ����?          ����?�  `                  @             ����?          ����?          ����?          ����?J  P                  @   	          ����?          ����?          ����?�  P                  @   
          ����?          ����?          ����?�  `                  @             ����?          ����?          ����?          ����?;  `                  @             ����?          ����?           ����?          ����?�  `                  @             ����?          ����?!             �?          ����?q  `                  @             ����?          ����?"             �?          ����?  `                  @             ����?          ����?#             �?          ����?�  `                  @             ����?          ����?$             �?          ����?B  `                  @             ����?          ����?%          ����?          ����?�  `                  @             ����?          ����?&          ����?          ����?x  P                  @             ����?          ����?'          ����?#  P                  @             ����?(          ����?          ����?�  `                  @             ����?          ����?)          ����?           ����?i  `                  @             ����?          ����?*          ����?!             �?  `                  Y@                �?              �?+             �?"             �?�  `                  Y@                �?!             �?,             �?#             �?:  `                  Y@                �?"             �?-             �?$             �?�  `                  Y@                �?#             �?.             �?%             �?p  `                  @             ����?$             �?/          ����?&          ����?  `                  @             ����?%          ����?0          ����?'          ����?�  P                  @             ����?&          ����?1          ����?Q  P                  @             ����?2          ����?)          ����?�  `                  @             ����?(          ����?3          ����?*          ����?�  `                  @              ����?)          ����?4          ����?+             �?2  `                  Y@   !             �?*             �?5             �?,             �?�  `                  Y@   "             �?+             �?6             �?-             �?h  `                  Y@   #             �?,             �?7             �?.             �?  `                  Y@   $             �?-             �?8             �?/             �?�  `                  @   %          ����?.             �?9          ����?0          ����?9  `                  @   &          ����?/          ����?:          ����?1          ����?�  P                  @   '          ����?0          ����?;          ����?  P                  @   (          ����?<          ����?3          ����?*  `                  @   )          ����?2          ����?=          ����?4          ����?�  `                  @   *          ����?3          ����?>          ����?5             �?`  `                  Y@   +             �?4             �??             �?6             �?�  `                  Y@   ,             �?5             �?@             �?7             �?�  `                  Y@   -             �?6             �?A             �?8             �?1  `                  Y@   .             �?7             �?B             �?9             �?�  `                  @   /          ����?8             �?C          ����?:          ����?g  `                  @   0          ����?9          ����?D          ����?;          ����?  P                  @   1          ����?:          ����?E          ����?�  P                  @   2          ����?F          ����?=          ����?X  `                  @   3          ����?<          ����?G          ����?>          ����?�  `                  @   4          ����?=          ����?H          ����??             �?�  `                  Y@   5             �?>             �?I             �?@             �?)  `                  Y@   6             �??             �?J             �?A             �?�  `                  Y@   7             �?@             �?K             �?B             �?_  `                  Y@   8             �?A             �?L             �?C             �?�  `                  @   9          ����?B             �?M          ����?D          ����?�  `                  @   :          ����?C          ����?N          ����?E          ����?0  P                  @   ;          ����?D          ����?O          ����?�
  P                  @   <          ����?P          ����?G          ����?�
  `                  @   =          ����?F          ����?Q          ����?H          ����?!
  `                  @   >          ����?G          ����?R          ����?I          ����?�	  `                  @   ?             �?H          ����?S          ����?J          ����?W	  `                  @   @             �?I          ����?T          ����?K          ����?�  `                  @   A             �?J          ����?U          ����?L          ����?�  `                  @   B             �?K          ����?V          ����?M          ����?(  `                  @   C          ����?L          ����?W          ����?N          ����?�  `                  @   D          ����?M          ����?X          ����?O          ����?^  P                  @   E          ����?N          ����?Y          ����?	  P                  @   F          ����?Z          ����?Q          ����?�  `                  @   G          ����?P          ����?[          ����?R          ����?O  `                  @   H          ����?Q          ����?\          ����?S          ����?�  `                  @   I          ����?R          ����?]          ����?T          ����?�  `                  @   J          ����?S          ����?^          ����?U          ����?   `                  @   K          ����?T          ����?_          ����?V          ����?�  `                  @   L          ����?U          ����?`          ����?W          ����?V  `                  @   M          ����?V          ����?a          ����?X          ����?�  `                  @   N          ����?W          ����?b          ����?Y          ����?�  P                  @   O          ����?X          ����?c          ����?7  @                  @   P          ����?[          ����?�  P                  @   Q          ����?Z          ����?\          ����?�  P                  @   R          ����?[          ����?]          ����?H  P                  @   S          ����?\          ����?^          ����?�  P                  @   T          ����?]          ����?_          ����?�  P                  @   U          ����?^          ����?`          ����?I  P                  @   V          ����?_          ����?a          ����?�   P                  @   W          ����?`          ����?b          ����?�   P                  @   X          ����?a          ����?c          ����?J   @                  @   Y          ����?b          ����?   @	  w #         � h  �&  �&  P&  @&  0&  �%  �%  �%  �%  �%  �%  Q%  A%  1%  �$  �$  �$  �$  �$  �$  R$  B$  2$  �#  �#  �#  �#  �#  c#  S#  C#  #  �"  �"  �"  �"  �"  �"  y"  D"  4"  $"  "  �!  �!  �!  �!  z!  j!  Z!  J!  !  !  �   �   �   �   �   �   K   ;   +      �  �  �  �  �  q  <  ,      �  �  �  �  r  b  R  B    �  �  �  �  �  �  x  C  3  #    �  �  �  �  y  i  Y  I      �  �  �  �  j  Z  J  :    �  �  �  �  �  �  p  ;  +      �  �  �  �  q  a  Q  A    �  �  �  �  �  �  w  B  2  "  �  �  �  �  �  x  h  3  #      �  �  �  �  i  Y  I  9    �  �  �  �  �    o  :  *    
  �  �  �  �  p  `  P      �  �  �  �  �  a  Q  A  1  �  �  �  �  �  �  w  g  2  "      �  �  �  �  h  X  H  8    �  �  �  �  �  ~  I  9  )  �  �  �  �  �    o  _  *    
  �  �  �  �  �  `  P  @  0  �  �  �  �  �  �  v  f  1  !      �  �  �  w  g  W  "      �  �  �  �  �  X  H  8  (  �
  �
  �
  �
  �
  ~
  n
  ^
  )
  
  	
  �	  �	  �	  �	  �	  _	  O	  ?	  /	  �  �  �  �  �  �  P  @  0     �  �  �  �  �  v  f  V  !      �  �  �  �  �  W  G  7  '  �  �  �  �  �  }  m  ]  (      �  �  �  ~  n  9  )    �  �  �  �    o  :  *    �  �  �  �  �  p  ;  +    �  �      �              	    �               j               T    0              *    " @( ! �@
!  w� �        	E                  ?                )      %   ! � {�        �                  �    �              �    "  ^    M`   7~" ~9`   " @( ! "  ^    N`$   " @( ! " @"  ^   N`   "  " ��� �        m                  g            
   Q    "     C`-   @%   ! w"    :& � {�        
+              
    %        @w� �        
5                  /                   ��        �                  �               �               �    .        %                  "F�" F"    =	I@" G�& &w& � {�        