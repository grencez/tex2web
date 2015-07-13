/**
 * This public domain code serves to:
 * Transform very simple LaTeX files into HTML files.
 *
 * Usage example:
 *   tex2web < in.tex > out.html
 **/

#include "cx/syscx.h"
#include "cx/fileb.h"

typedef struct HtmlState HtmlState;

struct HtmlState
{
  bool allgood;
  ujint nlines;
  bool eol;
  bool inparagraph;
  bool end_document;
  uint list_depth;
  bool list_item_open;
  Bool cram;
  OFile* of;
  uint nsections;
  uint nsubsections;
  OFile title;
  OFile author;
  OFile date;
  TableT(AlphaTab) search_paths;
};

static
  void
init_HtmlState (HtmlState* st, OFile* of)
{
  st->allgood = true;
  st->nlines = 1;
  st->eol = true;
  st->inparagraph = false;
  st->end_document = false;
  st->list_depth = 0;
  st->list_item_open = false;
  st->cram = false;
  st->of = of;
  st->nsections = 0;
  st->nsubsections = 0;
  init_OFile (&st->title);
  init_OFile (&st->author);
  init_OFile (&st->date);
  InitTable( st->search_paths );
}

static
  void
lose_HtmlState (HtmlState* st)
{
  lose_OFile (&st->title);
  lose_OFile (&st->author);
  lose_OFile (&st->date);
  for (i ; st->search_paths.sz)
    lose_AlphaTab (&st->search_paths.s[i]);
  LoseTable( st->search_paths );
}

static
  void
head_html (HtmlState* st)
{
  OFile* of = st->of;
#define W(s)  oput_cstr_OFile (of, s)
  //W("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
  //W("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML Basic 1.0//EN\" \"http://www.w3.org/TR/xhtml-basic/xhtml-basic10.dtd\">");
  W("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML-Print 1.0//EN\" \"http://www.w3.org/MarkUp/DTD/xhtml-print10.dtd\">");
  W("\n<html xmlns=\"http://www.w3.org/1999/xhtml\">");
  W("\n<head>");
  W("\n<meta http-equiv='Content-Type' content='text/html;charset=utf-8'/>");

  //W("<style media=\"screen\" type=\"text/css\">\n");
  W("\n<style type=\"text/css\">");
  W("\npre {");
  W("\n  padding-left: 3em;");
  W("\n  white-space: pre-wrap;");
  //W("\n  white-space: -moz-pre-wrap;");
  //W("\n  white-space: -o-pre-wrap;");
  W("\n  display: block;");
  W("\n}");
  W("\npre.cram {");
  W("\n  margin-top: -1em;");
  W("\n}");
  W("\nol.cram {");
  W("\n  margin-top: -1em;");
  W("\n}");
  W("\nul.cram {");
  W("\n  margin-top: -1em;");
  W("\n}");
  W("\np.cram {");
  W("\n  margin-top: -0.5em;");
  W("\n}");
  W("\nspan.underline { text-decoration: underline; }");
  W("\nspan.texttt,span.ttvbl {");
  W("\n  font-family:\"Courier New\", Monospace;");
  W("\n}");
  //W("\npre.shortb {");
  //W("\n  margin-bottom: -1em;");
  //W("\n}");
  W("\npre,code {");
  W("\n  background-color: #E2E2E2;");
  W("\n}");
  W("\ndiv {");
  W("\n  text-align: center;");
  W("\n}");
  W("\n</style>");

  W("\n<title>"); W(ccstr1_of_OFile (&st->title, 0)); W("</title>");
  W("\n</head>");
  W("\n<body>");

  W("\n<div>");
  W("\n<h1>"); W(ccstr1_of_OFile (&st->title, 0)); W("</h1>");
  W("\n<h3>"); W(ccstr1_of_OFile (&st->author, 0)); W("</h3>");
  W("\n<h3>"); W(ccstr1_of_OFile (&st->date, 0)); W("</h3>");
  W("\n</div>");
}

static
  void
foot_html (OFile* of)
{
  //W("<p><a href='http://validator.w3.org/check?uri=referer'>Valid XHTML-Print 1.0</a></p>\n");
  W("\n</body>");
  W("\n</html>\n");
#undef W
}


static
  void
escape_for_html (OFile* of, XFile* xf)
{
  //const char delims[] = "\"'&<>";
  const char delims[] = "\"&<>";
  char* s;
  char match = 0;
  while ((s = nextds_XFile (xf, &match, delims)))
  {
    oput_cstr_OFile (of, s);
    switch (match)
    {
    case '"':
      oput_cstr_OFile (of, "&quot;");
      break;
    case '\'':
      oput_cstr_OFile (of, "&#39;");
      break;
    case '&':
      oput_cstr_OFile (of, "&amp;");
      break;
    case '<':
      oput_cstr_OFile (of, "&lt;");
      break;
    case '>':
      oput_cstr_OFile (of, "&gt;");
      break;
    default:
      break;
    }
  }
}


static
  bool
hthead (HtmlState* st, XFile* xf)
{
  DeclLegit( good );
  while (good)
  {
    XFile olay[1];
    DoLegitLine( "" )
      !!getlined_XFile (xf, "\\");
    if (!good) {
      return false;
    }
    else if (skip_cstr_XFile (xf, "begin{document}")) {
      skipds_XFile (xf, WhiteSpaceChars);
      break;
    }
    else if (skip_cstr_XFile (xf, "title{"))
    {
      DoLegitLine( "no closing brace" )
        getlined_olay_XFile (olay, xf, "}");
      if (good) {
        escape_for_html (&st->title, olay);
      }
    }
    else if (skip_cstr_XFile (xf, "author{"))
    {
      DoLegitLine( "no closing brace" )
        getlined_olay_XFile (olay, xf, "}");
      if (good) {
        escape_for_html (&st->author, olay);
      }
    }
    else if (skip_cstr_XFile (xf, "date{"))
    {
      DoLegitLine( "no closing brace" )
        getlined_olay_XFile (olay, xf, "}");
      if (good) {
        escape_for_html (&st->date, olay);
      }
    }
  }
  if (good) {
    head_html (st);
  }
  st->allgood = st->allgood && good;
  return good;
}

static
  void
open_paragraph (HtmlState* st)
{
  if (st->inparagraph)  return;
  oput_cstr_OFile (st->of, "\n<p");
  if (st->cram) {
    oput_cstr_OFile (st->of, " class=\"cram\"");
  }
  oput_cstr_OFile (st->of, ">");
  st->inparagraph = true;
  st->eol = false;
  st->cram = true;
}

static
  void
close_paragraph (HtmlState* st)
{
  if (!st->inparagraph)  return;
  oput_cstr_OFile (st->of, "</p>");
  st->inparagraph = false;
  st->cram = false;
}

static
  void
open_list (HtmlState* st, const char* tag)
{
  bool cram = st->inparagraph && st->list_depth == 0;
  if (cram) {
    close_paragraph (st);
  }
  st->inparagraph = true;

  st->list_depth += 1;
  oput_cstr_OFile (st->of, "<");
  oput_cstr_OFile (st->of, tag);
  if (cram) {
    oput_cstr_OFile (st->of, " class=\"cram\"");
  }
  oput_cstr_OFile (st->of, ">");
  st->list_item_open = false;
}

static
  void
close_list (HtmlState* st, const char* tag)
{
  if (st->list_item_open) {
    oput_cstr_OFile (st->of, "</li>\n");
  }
  oput_cstr_OFile (st->of, "</");
  oput_cstr_OFile (st->of, tag);
  oput_cstr_OFile (st->of, ">");
  st->list_depth -= 1;
  if (st->list_depth > 0) {
    oput_cstr_OFile (st->of, "</li>\n");
  }
  else {
    st->inparagraph = false;
  }
  st->list_item_open = false;
}


// \\  -->  <br/>
// \   -->  &nbsp;
// \quicksec{TEXT}  -->  <b>TEXT.</b>
// \expten{NUM}  -->  &times;10<sup>NUM</sup>
// \textit{TEXT}  -->  <it>TEXT</it>
// \textbf{TEXT}  -->  <b>TEXT</b>
// \texttt{TEXT}  -->  <b>TEXT</b>
// \underline{TEXT}  -->  <u>TEXT</u>
// \ilcode{...}  -->  <code>...</code>
// \ilflag{...}
// \ilfile{...}
// \ilsym{...}
// \illit{...}
// \ilname{...}
// \ilkey{...}
// \ttvbl{...}
// $...$  -->  <i>...</i>
// \begin{code}\n...\n\end{code}  -->  <pre><code>...</code></pre>
// \href{URL}{TEXT}  -->  <a href='URL'>TEXT</a>
// \url{URL}  -->  <a href='URL'>URL</a>
// \section{TEXT}  -->  <h3>TEXT</h3>
// \subsection{TEXT}  -->  <h4>TEXT</h4>
static
  bool
htbody (HtmlState* st, XFile* xf, const char* pathname)
{
  DeclLegit( good );
  OFile* of = st->of;

  while (good && !st->end_document)
  {
    XFile olay[1];
    char match = 0;
    bool was_eol;
    if (!nextds_olay_XFile (olay, xf, &match, "\n\\%$"))
      break;

    //skipds_XFile (olay, WhiteSpaceChars);
    if (eq_cstr ("", ccstr_of_XFile (olay)))
    {
      if (match == '\n' && st->eol) {
        close_paragraph (st);
        st->cram = false;
      }
    }
    else
    {
      open_paragraph (st);
      if (st->eol)
        oput_cstr_OFile (of, "\n");
      escape_for_html (of, olay);
    }

    was_eol = st->eol;
    st->eol = false;

    if (!good) {
    }
    else if (match == '\n') {
      st->eol = true;
    }
    else if (match == '%') {
      st->eol = true;
      getline_XFile (xf);
    }
    else if (match == '$') {
      open_paragraph (st);
      oput_cstr_OFile (of, "<i>");
      DoLegitLine( "no closing dollar sign" )
        getlined_olay_XFile (olay, xf, "$");
      if (good) {
        escape_for_html (of, olay);
        oput_cstr_OFile (of, "</i>");
      }
    }
    else if (match == '\\') {
      if (skip_cstr_XFile (xf, "\\")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<br/>");
      }
      else if (skip_cstr_XFile (xf, " ")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "&nbsp;");
      }
      else if (skip_cstr_XFile (xf, "%")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "%");
      }
      else if (skip_cstr_XFile (xf, "begin{itemize}") ||
               skip_cstr_XFile (xf, "begin{itemize*}"))
      {
        open_list (st, "ul");
      }
      else if (skip_cstr_XFile (xf, "begin{enumerate}") ||
               skip_cstr_XFile (xf, "begin{enumerate*}"))
      {
        open_list (st, "ol");
      }
      else if (skip_cstr_XFile (xf, "end{itemize}") ||
               skip_cstr_XFile (xf, "end{itemize*}"))
      {
        close_list (st, "ul");
      }
      else if (skip_cstr_XFile (xf, "end{enumerate}") ||
               skip_cstr_XFile (xf, "end{enumerate*}"))
      {
        close_list (st, "ol");
      }
      else if (skip_cstr_XFile (xf, "item")) {
        if (st->list_item_open) {
          oput_cstr_OFile (of, "</li>\n");
        }
        oput_cstr_OFile (of, "<li>");
        st->list_item_open = true;
      }
      else if (skip_cstr_XFile (xf, "quicksec{")) {
        close_paragraph (st);
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, ".</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "expten{")) {
        open_paragraph (st);
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, "&times;10<sup>");
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</sup>");
        }
      }
      else if (skip_cstr_XFile (xf, "textit{")) {
        open_paragraph (st);
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, " <i>");
          htbody (st, olay, pathname);
          //escape_for_html (of, olay);
          oput_cstr_OFile (of, "</i>");
        }
      }
      else if (skip_cstr_XFile (xf, "textbf{")) {
        open_paragraph (st);
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, " <b>");
          htbody (st, olay, pathname);
          //escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "texttt{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, " <span class=\"texttt\">");
        DoLegitLine( "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</span>");
        }
      }
      else if (skip_cstr_XFile (xf, "underline{")) {
        open_paragraph (st);
        DoLegitLine( "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, " <span class=\"underline\">");
          htbody (st, olay, pathname);
          //escape_for_html (of, olay);
          oput_cstr_OFile (of, "</span>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilcode{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<code>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</code>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilflag{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilfile{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<i>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</i>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilsym{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "illit{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<i>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</i>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilname{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilkey{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ttvbl{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, " <span class=\"ttvbl\">");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</span>");
        }
      }
      else if (skip_cstr_XFile (xf, "begin{code}\n"))
      {
        Bool cram = false;
        if (st->inparagraph || st->cram) {
          cram = true;
        }
        close_paragraph (st);
        oput_cstr_OFile (of, "\n<pre");
        if (cram)
          oput_cstr_OFile (of, " class=\"cram\"");
        oput_cstr_OFile (of, "><code>");

        DoLegitLine( "Need \\end{code} for \\begin{code}!" )
          getlined_olay_XFile (olay, xf, "\n\\end{code}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</code></pre>");
        }
        st->cram = true;
      }
      else if (skip_cstr_XFile (xf, "codeinputlisting{"))
      {
        Bool cram = false;
        XFileB xfileb[1];
        init_XFileB (xfileb);
        if (st->inparagraph || st->cram) {
          cram = true;
        }
        close_paragraph (st);
        oput_cstr_OFile (of, "\n<pre");
        if (cram)
          oput_cstr_OFile (of, " class=\"cram\"");
        oput_cstr_OFile (of, "><code>");

        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");

        DoLegit( "cannot open listing" )
        {
          const char* filename = ccstr_of_XFile (olay);
          good = open_FileB (&xfileb->fb, pathname, filename);
        }
        if (good)
          escape_for_html (of, &xfileb->xf);
        oput_cstr_OFile (of, "</code></pre>");
        lose_XFileB (xfileb);
      }
      else if (skip_cstr_XFile (xf, "section{")) {
        close_paragraph (st);
        oput_cstr_OFile (of, "\n<h3>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_uint_OFile (of, ++ st->nsections);
          st->nsubsections = 0;
          oput_cstr_OFile (of, ". ");
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</h3>");
        }
      }
      else if (skip_cstr_XFile (xf, "subsection{")) {
        close_paragraph (st);
        oput_cstr_OFile (of, "\n<h4>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_uint_OFile (of, st->nsections);
          oput_cstr_OFile (of, ".");
          oput_uint_OFile (of, ++ st->nsubsections);
          oput_cstr_OFile (of, ". ");
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</h4>");
        }
      }
      else if (skip_cstr_XFile (xf, "href{")) {
        if (was_eol && st->inparagraph)
          oput_char_OFile (of, '\n');
        open_paragraph (st);
        oput_cstr_OFile (of, "<a href='");
        DoLegitLine( "no closing/open for href" )
          getlined_olay_XFile (olay, xf, "}{");

        DoLegit( "no closing brace" )
        {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "'>");
          good = getlined_olay_XFile (olay, xf, "}");
        }
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</a>");
        }
      }
      else if (skip_cstr_XFile (xf, "url{")) {
        XFile olay2[1];
        if (was_eol && st->inparagraph)
          oput_char_OFile (of, '\n');
        open_paragraph (st);
        oput_cstr_OFile (of, "<a href='");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          *olay2 = *olay;
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "'>");
          escape_for_html (of, olay2);
          oput_cstr_OFile (of, "</a>");
        }
      }
      else if (skip_cstr_XFile (xf, "caturl{")) {
        if (was_eol && st->inparagraph)
          oput_char_OFile (of, '\n');
        open_paragraph (st);
        oput_cstr_OFile (of, "<a href='");
        DoLegitLine( "no closing/open for caturl" )
          getlined_olay_XFile (olay, xf, "}{");

        DoLegit( "no closing brace" )
        {
          escape_for_html (of, olay);
          good = getlined_olay_XFile (olay, xf, "}");
        }
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "'>");
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</a>");
        }
      }
      else if (skip_cstr_XFile (xf, "input{")) {
        XFileB xfb[1];
        AlphaTab filename[1];
        init_XFileB (xfb);
        init_AlphaTab (filename);
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");

        DoLegit( "Cannot open file!" )
        {
          cat_cstr_AlphaTab (filename, ccstr_of_XFile (olay));
          cat_cstr_AlphaTab (filename, ".tex");
          good = open_FileB (&xfb->fb, pathname, ccstr_of_AlphaTab (filename));
          if (!good) {
            for (uint i = 0; i < st->search_paths.sz && !good; ++i) {
              good = open_FileB (&xfb->fb,
                                 ccstr_of_AlphaTab (&st->search_paths.s[i]),
                                 ccstr_of_AlphaTab (filename));
            }
          }
        }
        if (good) {
          htbody (st, &xfb->xf, ccstr_of_AlphaTab (&xfb->fb.pathname));
        }
        else {
          DBog0( pathname );
          DBog0( ccstr_of_AlphaTab (filename) );
        }
        lose_XFileB (xfb);
        lose_AlphaTab (filename);
      }
      else if (skip_cstr_XFile (xf, "end{document}"))
      {
        close_paragraph (st);
        st->end_document = true;
        break;
      }
      else {
        char match = 0;
        char* s = nextds_XFile (xf, &match, "{}()[] \n\t");
        if (s) {
          DBog1( "I don't yet understand; \\%s", s );
        }
        if (match == '{') {
          nextds_XFile (xf, 0, "}");
        }
      }
    }
  }
  st->allgood = st->allgood && good;
  return good;
}

  int
main (int argc, char** argv)
{
  DeclLegit( good );
  int argi =
    (init_sysCx (&argc, &argv),
     1);
  XFileB xfb[1];
  OFileB ofb[1];
  XFile* xf = stdin_XFile ();
  OFile* of = stdout_OFile ();
  HtmlState st[1];

  init_XFileB (xfb);
  init_OFileB (ofb);

  init_HtmlState (st, of);

  while (good && argi < argc)
  {
    if (eq_cstr ("-x", argv[argi])) {
      DoLegitLine( "open file for reading" )
        open_FileB (&xfb->fb, 0, argv[++argi]);
      if (good) {
        ++ argi;
        xf = &xfb->xf;
      }
    }
    else if (eq_cstr ("-o", argv[argi])) {
      DoLegitLine( "open file for writing" )
        open_FileB (&ofb->fb, 0, argv[++argi]);
      if (good) {
        ++ argi;
        st->of = &ofb->of;
      }
    }
    else if (eq_cstr ("-I", argv[argi])) {
      AlphaTab* path = Grow1Table( st->search_paths );
      *path = dflt_AlphaTab ();
      argi += 1;
      if (argi == argc) {
        failout_sysCx ("no argument given for -I");
      }
      cat_cstr_AlphaTab (path, argv[argi++]);
    }
    else {
      good = false;
    }
  }
  if (!good)
    return 1;

  hthead (st, xf);
  htbody (st, xf, ccstr_of_AlphaTab (&xfb->fb.pathname));
  foot_html (st->of);
  if (!st->end_document) {
    good = false;
  }
  good = good && st->allgood;

  lose_HtmlState (st);
  lose_XFileB (xfb);
  lose_OFileB (ofb);
  lose_sysCx ();
  good = 1;
  return good ? 0 : 1;
}

