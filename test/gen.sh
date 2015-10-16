#!/bin/sh

exepath=$(dirname "$0")
tex2web=${exepath}/../bin/tex2web
example=${exepath}/../example
expect=${exepath}/expect
css='-css style.css'

$tex2web -x "$example/hello.tex" -o "$expect/hello.html" $css
$tex2web -x "$example/macro.tex" -o "$expect/macro.html" -def pathname ../my/path $css
$tex2web -x "$example/table.tex" -o "$expect/table.html" $css
$tex2web -x "$example/toc.tex" -o "$expect/toc.html" $css
$tex2web -o-css "$expect/style.css"

