
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
  W("\np.cram {");
  W("\n  margin-top: -0.5em;");
  W("\n}");
  W("\nspan.ttvbl {");
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
  Sign good = 1;
  while (good)
  {
    XFile olay[1];
    DoLegit( good, "" )
      good = !!getlined_XFile (xf, "\\");
    if (!good) {
      return false;
    }
    else if (skip_cstr_XFile (xf, "begin{document}")) {
      skipds_XFile (xf, WhiteSpaceChars);
      break;
    }
    else if (skip_cstr_XFile (xf, "title{"))
    {
      DoLegit( good, "no closing brace" )
        good = getlined_olay_XFile (olay, xf, "}");
      if (good) {
        escape_for_html (&st->title, olay);
      }
    }
    else if (skip_cstr_XFile (xf, "author{"))
    {
      DoLegit( good, "no closing brace" )
        good = getlined_olay_XFile (olay, xf, "}");
      if (good) {
        escape_for_html (&st->author, olay);
      }
    }
    else if (skip_cstr_XFile (xf, "date{"))
    {
      DoLegit( good, "no closing brace" )
        good = getlined_olay_XFile (olay, xf, "}");
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


// \\  -->  <br/>
// \   -->  &nbsp;
// \quicksec{TEXT}  -->  <b>TEXT.</b>
// \expten{NUM}  -->  &times;10<sup>NUM</sup>
// \textit{TEXT}  -->  <it>TEXT</it>
// \textbf{TEXT}  -->  <b>TEXT</b>
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
  Sign good = 1;
  OFile* of = st->of;

  while (good && !st->end_document)
  {
    XFile olay[1];
    char match = 0;
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
      DoLegit( good, "no closing dollar sign" )
        good = getlined_olay_XFile (olay, xf, "$");
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
      else if (skip_cstr_XFile (xf, "begin{itemize}")) {
      }
      else if (skip_cstr_XFile (xf, "begin{itemize*}")) {
      }
      else if (skip_cstr_XFile (xf, "end{itemize}")) {
        oput_cstr_OFile (of, "<br/>");
      }
      else if (skip_cstr_XFile (xf, "end{itemize*}")) {
        oput_cstr_OFile (of, "<br/>");
      }
      else if (skip_cstr_XFile (xf, "item")) {
        if (!st->inparagraph) {
          open_paragraph (st);
          oput_cstr_OFile (of, "-");
        }
        else {
          oput_cstr_OFile (of, "\n<br/>-");
        }
      }
      else if (skip_cstr_XFile (xf, "quicksec{")) {
        close_paragraph (st);
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, ".</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "expten{")) {
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, "&times;10<sup>");
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</sup>");
        }
      }
      else if (skip_cstr_XFile (xf, "textit{")) {
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, " <i>");
          htbody (st, olay, pathname);
          //escape_for_html (of, olay);
          oput_cstr_OFile (of, "</i>");
        }
      }
      else if (skip_cstr_XFile (xf, "textbf{")) {
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, " <b>");
          htbody (st, olay, pathname);
          //escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "underline{")) {
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, " <u>");
          htbody (st, olay, pathname);
          //escape_for_html (of, olay);
          oput_cstr_OFile (of, "</u>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilcode{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<code>");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</code>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilflag{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilfile{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<i>");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</i>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilsym{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "illit{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<i>");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</i>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilname{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilkey{")) {
        open_paragraph (st);
        //oput_cstr_OFile (of, "<b>");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay);
          //oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ttvbl{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, " <span class=\"ttvbl\">");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
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

        DoLegit( good, "Need \\end{code} for \\begin{code}!" )
          good = getlined_olay_XFile (olay, xf, "\n\\end{code}");
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

        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");

        DoLegit( good, "cannot open listing" )
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
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
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
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
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
        open_paragraph (st);
        oput_cstr_OFile (of, "\n<a href='");
        DoLegit( good, "no closing/open for href" )
          good = getlined_olay_XFile (olay, xf, "}{");

        DoLegit( good, "no closing brace" )
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
        open_paragraph (st);
        oput_cstr_OFile (of, "\n<a href='");
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          *olay2 = *olay;
          escape_for_html (of, olay);
          oput_cstr_OFile (of, "'>");
          escape_for_html (of, olay2);
          oput_cstr_OFile (of, "</a>");
        }
      }
      else if (skip_cstr_XFile (xf, "caturl{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "\n<a href='");
        DoLegit( good, "no closing/open for caturl" )
          good = getlined_olay_XFile (olay, xf, "}{");

        DoLegit( good, "no closing brace" )
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
        DoLegit( good, "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");

        DoLegit( good, "Cannot open file!" )
        {
          cat_cstr_AlphaTab (filename, ccstr_of_XFile (olay));
          cat_cstr_AlphaTab (filename, ".tex");
          good = open_FileB (&xfb->fb, pathname, cstr_of_AlphaTab (filename));
          if (!good) {
            for (uint i = 0; i < st->search_paths.sz && !good; ++i) {
              good = open_FileB (&xfb->fb,
                                 cstr_of_AlphaTab (&st->search_paths.s[i]),
                                 cstr_of_AlphaTab (filename));
            }
          }
        }
        if (good) {
          htbody (st, &xfb->xf, cstr_of_AlphaTab (&xfb->fb.pathname));
        }
        else {
          DBog0( pathname );
          DBog0( cstr_of_AlphaTab (filename) );
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
  Sign good = 1;
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
      DoLegit( good, "open file for reading" )
        good = open_FileB (&xfb->fb, 0, argv[++argi]);
      if (good) {
        ++ argi;
        xf = &xfb->xf;
      }
    }
    else if (eq_cstr ("-o", argv[argi])) {
      DoLegit( good, "open file for writing" )
        good = open_FileB (&ofb->fb, 0, argv[++argi]);
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
  htbody (st, xf, cstr_of_AlphaTab (&xfb->fb.pathname));
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

