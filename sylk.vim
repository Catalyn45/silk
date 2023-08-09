if exists("b:current_syntax")
  finish
endif

syn match SylkIdentifier "\<[a-zA-Z_][a-zA-Z0-9_]*\>"
syn match SylkFunction "\<[a-zA-Z_][a-zA-Z0-9_]*\>" contained

syn region SylkString start='"' end='"'
syn region SylkString start="'" end="'"
syn region SylkComment start="#" end="$"
syn match SylkNumber '\d\+' display
syn match SylkNumber '[-+]\d\+' display

syn keyword SylkDeclarations var class nextgroup=SylkIdentifier skipwhite
syn keyword SylkDeclarations def nextgroup=SylkFunction skipwhite

syn keyword SylkConditionals if else
syn keyword SylkRepeats while
syn keyword SylkSpecial self
syn keyword SylkKeywords return

syn match SylkParants /[()]/ contains=SylkParants
syn match SylkBrackets /[{}]/ contains=SylkBrackets
syn match SylkSquare /[][[]/ contains=SylkSquare

syn region SylkBlock start="{" end="}" fold transparent

let b:current_syntax = "sylk"

hi def link SylkDeclarations Type
hi def link SylkKeywords Keyword

hi def link SylkIdentifier Identifier

hi def link SylkString String
hi def link SylkNumber Number

hi def link SylkSpecial Special
hi def link SylkFunction Function
hi def link SylkConditionals Conditional
hi def link SylkRepeats Repeat

hi def link SylkBrackets Delimiter
hi def link SylkParants Delimiter
hi def link SylkSquare Delimiter

hi def link SylkComment Comment
