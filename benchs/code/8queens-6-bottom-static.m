meld fil       $                                               	       	                     
       
                                                                                                                                                                                                                                                                                                                                                                       !       !       "       "                     #       #                      	                init -o axioms�   propagate-left(Nodes, Coords) -o test-and-send-down(Nodes, Coords), {L | !left(L), 
			L != host-id | propagate-left(Nodes, Coords)@L}.�   propagate-right(Nodes, Coords) -o test-and-send-down(Nodes, Coords), {R | !right(R), 
			R != host-id | propagate-right(Nodes, Coords)@R}.T   test-and-send-down(Nodes, Coords), !coord(X, Y) -o test-y(Y, Coords, Nodes, Coords).v   test-y(Y, MV3, Nodes, Coords), !coord(OX, OY), test-nil(MV3) -o test-diag-left(OX - 1, OY - 1, Coords, Nodes, Coords).�   test-y(MV24, MV25, MV26, MV27), MV29 = head(MV28), MV28 = tail(MV25), not(test-nil(MV25)), 
			not(test-nil(MV28)) -o (MV24 = MV29 -o 1), OR (RestCoords = tail(MV28), MV24 != MV29 -o test-y(MV24, RestCoords, MV26, MV27)).�   test-diag-left(X, Y, MV1, Nodes, Coords), !coord(OX, OY), (X < 0) || (Y < 0) -o test-diag-right(OX - 1, OY + 1, Coords, Nodes, Coords).5  test-diag-left(MV30, MV31, MV32, MV33, MV34), MV37 = head(MV36), MV36 = tail(MV32), MV35 = head(MV32), 
			not(test-nil(MV32)), not(test-nil(MV36)) -o (MV30 = MV35, MV31 = MV37 -o 1), OR (RestCoords = tail(MV36), (MV30 != MV35) || (MV31 != MV37) -o test-diag-left(MV30 - 1, MV31 - 1, RestCoords, MV33, MV34)).�   test-diag-right(X, Y, MV16, Nodes, Coords), !coord(OX, OY), (X < 0) || (Y >= 6), test-nil(MV16) -o 
			send-down(cons(host-id,Nodes), cons(OX,cons(OY,Coords))).7  test-diag-right(MV38, MV39, MV40, MV41, MV42), MV45 = head(MV44), MV44 = tail(MV40), MV43 = head(MV40), 
			not(test-nil(MV40)), not(test-nil(MV44)) -o (MV38 = MV43, MV39 = MV45 -o 1), OR (RestCoords = tail(MV44), (MV38 != MV43) || (MV39 != MV45) -o test-diag-right(MV38 - 1, MV39 + 1, RestCoords, MV41, MV42)).T   send-down(Nodes, Coords), !coord(MV23, MV2), MV23 = 5 -o final-state(Nodes, Coords).�   send-down(Nodes, Coords) -o {B | !down-right(B), B != host-id | 
			propagate-right(Nodes, Coords)@B}, {B | !down-left(B), B != host-id | 
			propagate-left(Nodes, Coords)@B}.           �                  _init                                                               set-priority                                                        setcolor                                                            setedgelabel                                                        write-string                                                        add-priority                                                         schedule-next                                                       setcolor2                                                            stop-program                                                        set-default-priority                                                 set-moving                                                           set-static                                                          set-affinity                                                        set-cpu                                                              remove-priority                                                      left                                                                 right                                                                down-right                                                           down-left                                                            coord                                                                propagate-left                                                       propagate-right                                                      test-and-send-down                                                    test-y                                                                test-diag-left                                                        test-diag-right                                                      send-down                                                            final-state                                                      ���������                                                                                                                                                                 �   � 
�  $      q   �   ;  �    j  �  4  �  �  c  �  -  �  �  \  �  &  �  �  U  �  	  �	  �	  N
  �
    }  �  G  �    v  �                 /�.      �?�@p pw2                                         �                /�.      �?�2          	                             o                /�.      �?�2          
                            
                /�.      �?�2                                      �                /�.      �?�2                                      @                /�.      �?�2                 	                     �                /�.       @�2                                      v               /�.       @�2                                                    /�.       @�2                              	       �
               /�.       @�2                              
       G
               /�.       @�2         
              	              �	               /�.       @�2                       
              }	                /�.      @�2                                      	               /�.      @�2                                     �               /�.      @�2                                     N               /�.      @�2                                     �               /�.      @�2                                     �               /�.      @�2                                                     /�.      @�2                                      �               /�.      @�2                                     U               /�.      @�2                                     �               /�.      @�2                                     �               /�.      @�2                                     &               /�.      @�2                                     �                /�.      @�2                                       \               /�.      @�2         !                            �               /�.      @�2         "                            �               /�.      @�2         #                            -               /�.      @�2                                      �               /�.      @�2                !                     c                /�.      @�2                                      �               /�.      @�2                                      �               /�.      @�2                                !       4               /�.      @�2         !       !               "       �                /�.      @�2         "       "       !       #       j                /�.      @�2         #       #       "       #          #         � �   �  �  �  �  �  z  q  h          �  �  �  �  T  K  B  9  �  �  �  �  �  �  x  o  %      
  �  �  �  �  [  R  I  @  �
  �
  �
  �
  �
  �
  
  v
  ,
  #
  
  
  �	  �	  �	  �	  b	  Y	  P	  G	  �  �  �  �  �  �  �  }  3  *  !    �  �  �  �  i  `  W  N    �  �  �  �  �  �  �  :  1  (    �  �  �  �  p  g  ^  U      �  �  �  �  �  �  A  8  /  &  �  �  �  �  w  n  e  \    	     �  �  �  �  �  H  ?  6  -  �  �  �  �  ~  u  l  c        �        o                  i    @%   % wA              ;    " 78`   @%   % " �� �         o                  i    @%   % wA              ;    " 78`   @%   % " �� �         N                  H               2    @! % %  % w� �         o                  i              Q    @"    :& "   :&% % % w� �         P                  J   Z S"  ;`	   � �"  <`   Y' { �         �                  �    "      >`   "     >`W              Q    @"    :& "   =&% % % w� �         �                  �   Z ST "  ;`   " ;`	   � �"  <`   " <`4   Y"     :&  "    :& ' { �         �                  �   "      >`   "    ?`M              G    @" 7_' " "_ " _ 'w� �         �   	               �   Z ST "  ;`   " ;`	   � �"  <`   " <`4   Y"     :&  "    =& ' { �         J   
               D               .       @%   % w� �         �                  �    A              ;    " 78`   @%   % " �A              ;    " 78`   @%   % " �� �         