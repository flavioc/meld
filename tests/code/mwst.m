meld fil       *                                                                                        	                _init -o node-axioms.�   start(), state(SN), !edge(B, W), edgetype(MV14, Type), 
			SN = 0, MV14 = B -o state(2), edgetype(B, 2), connect(host-id, 0)@B, 
			findcount(0), level(0).�   connect(J, L), !level(LN), edgetype(MV15, OldType), !edge(MV16, Weight), 
			!state(SN), !core(FN), L < LN, MV15 = J, MV16 = J -o 
			edgetype(J, 2), inc-if-find(SN), initiate(host-id, LN, FN, SN)@J.@   inc-if-find(State), findcount(N), State = 1 -o findcount(N + 1).$   inc-if-find(State), State != 1 -o 1.�   connect(J, L), !level(LN), !edgetype(MV17, NotBasic), !edge(MV18, Wj), 
			L >= LN, NotBasic != 0, MV17 = J, MV18 = J -o initiate(host-id, LN + 1, Wj, 1)@J._  initiate(J, L, F, S), level(LN), state(SN), core(FN), 
			inbranch(MV1), best-edge(MV2), best-wt(MV3) -o level(L), state(S), 
			core(F), inbranch(J), best-edge(host-id), best-wt(999999.9), do-test-if-find(S), 
			{B, W, Branch | !edge(B, W), edgetype(B, Branch), B != J, Branch = 2 | 
			edgetype(B, 2), inc-if-find(S), initiate(host-id, L, F, S)@B}.E   do-test-if-find(MV29) -o (MV29 = 1 -o dotest()), OR (MV29 != 1 -o 1).�   dotest(), !edge(B, W), edgetype(MV19, Basic), test-edge(MV4), 
			!level(LN), !core(FN), Basic = 0, MV19 = B -o test-edge(B), 
			edgetype(B, 0), test(host-id, LN, FN)@B.;   dotest(), test-edge(MV5) -o test-edge(host-id), doreport().�   test(J, L, F), !level(LN), !core(FN), !test-edge(TestEdge), 
			L <= LN, F = FN -o maysendreject(TestEdge, J), {Basic | edgetype(J, Basic), 
			Basic = 0 | edgetype(J, 1)}.5   maysendreject(TestEdge, J), TestEdge = J -o dotest()._   maysendreject(TestEdge, J), !edge(MV20, MV6), TestEdge != J, MV20 = J -o 
			reject(host-id)@J.4   maysendreject(TestEdge, J), TestEdge = host-id -o 1.l   test(J, L, F), !edge(MV21, MV7), !level(LN), !core(FN), 
			L <= LN, FN != F, MV21 = J -o accept(host-id)@J.I   accept(J), test-edge(MV8) -o test-edge(host-id), may-change-best-edge(J).�   may-change-best-edge(J), best-wt(BW), !edge(MV22, ThisW), best-edge(MV9), 
			ThisW < BW, MV22 = J -o best-edge(J), best-wt(ThisW), doreport().c   may-change-best-edge(J), !best-wt(BW), !edge(MV23, ThisW), ThisW >= BW, 
			MV23 = J -o doreport().d   reject(J) -o dotest(), {W, Basic | !edge(J, W), 
			edgetype(J, Basic), Basic = 0 | edgetype(J, 1)}.�   doreport(), findcount(FC), test-edge(Nil), state(SN), 
			!inbranch(In), !edge(MV24, MV10), !best-wt(BW), FC = 0, Nil = host-id, 
			MV24 = In -o findcount(0), test-edge(host-id), state(2), report(host-id, BW)@In.   doreport() -o 1.�   report(J, W), !inbranch(InBranch), findcount(FindCount), InBranch != J -o 
			findcount(FindCount - 1), may-set-best-edge(J, W).j   may-set-best-edge(J, W), best-wt(BW), best-edge(MV11), W < BW -o 
			best-wt(W), best-edge(J), doreport().=   may-set-best-edge(J, W), !best-wt(BW), W >= BW -o doreport().
   -o (report(J, W), inbranch(InBranch), state(SN), best-wt(BW), 
			InBranch = J, SN != 1, W > BW -o inbranch(InBranch), state(SN), 
			best-wt(BW), do-change-root()), OR (report(J, W), inbranch(InBranch), state(SN), best-wt(BW), 
			InBranch = J, SN != 1, W <= BW, W = 999999.9, BW = 999999.9 -o 
			inbranch(InBranch), state(SN), best-wt(999999.9), halt()), OR (report(J, W), inbranch(InBranch), state(SN), best-wt(BW), 
			InBranch = J, SN != 1, W <= BW, W != 999999.9 -o inbranch(InBranch), 
			state(SN), best-wt(BW)).�   do-change-root(), !best-edge(B), !edge(MV25, MV12), edgetype(MV26, Branch), 
			Branch = 2, MV25 = B, MV26 = B -o edgetype(B, 2), change-root()@B.�   do-change-root(), !best-edge(B), !edge(MV27, MV13), edgetype(MV28, Branch), 
			!level(LN), Branch != 2, MV27 = B, MV28 = B -o edgetype(B, 2), 
			connect(host-id, LN)@B."   change-root() -o do-change-root().           �                   _init                                                                setcolor                                                             setedgelabel                                                         write-string                                                         setcolor2                                                             stop-program                                                          set-priority                                                          add-priority                                                          schedule-next                                                         set-default-priority                                                  set-moving                                                            set-static                                                           set-affinity                                                         set-cpu                                                               remove-priority                                                      state                                                                  edge                                                                  start                                                                edgetype                                                             connect                                                              findcount                                                            level                                                                 initiate                                                             inc-if-find                                                           core                                                                 inbranch                                                              best-wt                                                              best-edge                                                            do-test-if-find                                                       dotest                                                                report                                                                test                                                                 test-edge                                                            accept                                                               maysendreject                                                        reject                                                               may-change-best-edge                                                  doreport                                                              may-set-best-edge                                                     do-change-root                                                        halt                                                                  change-root                                                      ��������   t                   n   � 
     �  U  e   �      �   _             ����?                     @33�?                     ���@           �  A             ����?                      ���@            ;  A             @33�?                     `ff@           �   _             `ff@                     ��� @                     ���@           �   A             ��� @                      ���@            K   A             ���@                     ���@              @       w @ (  w @ -   ��.A  w @  (  w @ (  w @ -          w @ w #         �      �  �  �  �  �  �  �  �  p  c  R  E  4      �   �   �   �   �   �   �   �   s   b   U   D       �                  �               �               |                    `         {%     {@(     " @     w@     w� �        �                  �               �    " " >`�               �                  }                 c               M    %      {@!  w@( ! ! ! "  � �        O                  I                  -    "    =& {� �        3                  -    "     <`	   � �        �                  �               �    " " ?`|               v      "    <`N               H      @( "    =&!   "  � �        c                 ]              G              1                                          �               �    !  {!  {!  {%   {( {-   ��.A {@!  w�              �    " "  	8	``               Z        @	%  	   	z	@	!  	w	@	( 	! 	! 	! 	" 
	
��� �        O                  I    "     ;`   @w� �"     <`	   � �        �                  �               �                �                     q               [               E    %  {%      {@( ! ! " � �         ?   	               9                #    ( {@%w� �         �   
               �               �    " " B`�              �    " " J`h               b    @"%  %  w:               4          @%      z��� �         6              "    0    "  " 9`   @w� �        "X              "    R    "  " 8`0               *     @#( " � �        "/              "    )    "  79`	   � �        "�                  �                z                 `    " " B`>              8    " " K`   @!( "  � �        D              !    >                (    ( {@$%   w� �         !�              $    �               n                X      "" L`2              ,    %   {! {@%w� �        $f              $    `               J                4      "" O`   @%w� �        $x              #    r    @wT               N                  4          @%      z��� �        #�              %    �               �                    �    " 79`�              �               r                \                B         {( {    {@( ! " � �        %               %        � �        %�                  z               d    " "  8`B              <    "    :& {@&%   ! w� �        p              &    j               T    " " L`2              ,    !  {%   {@%w� �        &L              &    F               0    " " O`   @%w� �        &�                 �                z                 `    "    <`<              6    " " N`   {{{@'w� �            �       ��.A            �                 u    "    <`Q              K        ��.A" " M`   {{-   ��.A {@(w� �            �    " .   ��.AK`{               u                 [    "    <`7              1    " " M`   {{{� �        �              '    �               k                U                 ;        %     {@)" � �        '�              '    �               �                �                 g     "   <`?              9    %     {@( ! " � �        '$              )        @'w� �        )