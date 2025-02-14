; =============================================================================
; Fuzzy conjunction introduction rule
;
; A1
; ...
; An
; |-
; AndLink
;   A1
;   ...
;   An
;
; Where A1 to An are atoms with a fuzzy TV
; -----------------------------------------------------------------------------

(use-modules (srfi srfi-1))

;; Generate variable (Variable prefix + "-" + to_string(i))
(define (gen-variable prefix i)
  (Variable (string-append prefix "-" (number->string i))))

;; Generate a list of variables (Variable prefix + "-" + to_string(n))
(define (gen-variables prefix n)
  (if (= n 0)
      ;; Base case
      '()
      ;; Recursive case
      (append (gen-variables prefix (- n 1))
              (list (gen-variable prefix (- n 1))))))

;; Generate a fuzzy conjunction introduction rule for an n-ary
;; conjunction
(define (gen-fuzzy-conjunction-introduction-rule nary)
  (let* ((variables (gen-variables "$X" nary))
         (EvaluationT (Type "EvaluationLink"))
         (InheritanceT (Type "InheritanceLink"))
         (type (TypeChoice EvaluationT InheritanceT))
         (gen-typed-variable (lambda (x) (TypedVariable x type)))
         (vardecl (VariableSet (map gen-typed-variable variables)))
         (pattern (Present variables))
         (rewrite (ExecutionOutput
                    (GroundedSchema "scm: fuzzy-conjunction-introduction")
                    ;; We wrap the variables in Set because the order
                    ;; doesn't matter and that way alpha-conversion
                    ;; works better.
                    (List (And variables) (Set variables)))))
    (Bind
      vardecl
      pattern
      rewrite)))

;; Return true if all elements of the list are unique
(define (is-set l)
  (cond ((null? l) #t)
        ((member (car l) (cdr l)) #f)
        (else (is-set (cdr l)))))

;; Check that they all are different, and have positive confidences
(define (fuzzy-conjunction-introduction-precondition S)
  (bool->tv (is-confident-enough-set (cog-outgoing-set S))))

(define (is-confident-enough-set andees)
  (let* ((confident-enough (lambda (A) (> (cog-confidence A) 0))))
    (and (is-set andees)
         (every confident-enough andees))))

(define (fuzzy-conjunction-introduction A S)
  (let* ((andees (cog-outgoing-set S))
         (min-s-atom (min-element-by-key andees cog-mean))
         (min-c-atom (min-element-by-key andees cog-confidence))
         (min-s (cog-mean min-s-atom))
         (min-c (cog-confidence min-c-atom)))
    (if (is-confident-enough-set andees)       ; only introduce meaningful
                                               ; conjunction of unique andees
        (cog-set-tv! A (stv min-s min-c)))))

;; Name the rules
;;
;; Lame enumeration, maybe scheme can do better?
(define fuzzy-conjunction-introduction-1ary-rule-name
  (DefinedSchema "fuzzy-conjunction-introduction-1ary-rule"))
(DefineLink
  fuzzy-conjunction-introduction-1ary-rule-name
  (gen-fuzzy-conjunction-introduction-rule 1))
(define fuzzy-conjunction-introduction-2ary-rule-name
  (DefinedSchema "fuzzy-conjunction-introduction-2ary-rule"))
(DefineLink
  fuzzy-conjunction-introduction-2ary-rule-name
  (gen-fuzzy-conjunction-introduction-rule 2))
(define fuzzy-conjunction-introduction-3ary-rule-name
  (DefinedSchema "fuzzy-conjunction-introduction-3ary-rule"))
(DefineLink
  fuzzy-conjunction-introduction-3ary-rule-name
  (gen-fuzzy-conjunction-introduction-rule 3))
(define fuzzy-conjunction-introduction-4ary-rule-name
  (DefinedSchema "fuzzy-conjunction-introduction-4ary-rule"))
(DefineLink
  fuzzy-conjunction-introduction-4ary-rule-name
  (gen-fuzzy-conjunction-introduction-rule 4))
(define fuzzy-conjunction-introduction-5ary-rule-name
  (DefinedSchema "fuzzy-conjunction-introduction-5ary-rule"))
(DefineLink
  fuzzy-conjunction-introduction-5ary-rule-name
  (gen-fuzzy-conjunction-introduction-rule 5))
