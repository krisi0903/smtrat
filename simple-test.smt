(set-logic QF_NRA)
(declare-const x Real)
(declare-const y Real)
(declare-const z Real)
(declare-const u Real)
(declare-const v Real)
(declare-const w Real)
(assert (= (* x y) (* 2 (* x z))))
(assert (= (* w y) (* 2 (* w z))))
(check-sat)
; unsat
(exit)