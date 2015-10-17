/**
 * This public domain code serves to:
 * Transform very simple LaTeX files into HTML files.
 *
 * Usage example:
 *   tex2web < in.tex > out.html
 **/

#include "cx/syscx.h"
#include "cx/fileb.h"
#include "cx/associa.h"

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
  bool show_toc;
  ujint toc_pos;
  OFile* ofile;
  uint nsections;
  uint nsubsections;
  OFile title[1];
  OFile author[1];
  OFile date[1];
  OFile toc_ofile[1];
  OFile body_ofile[1];
  TableT(AlphaTab) search_paths;
  Associa macro_map;
  AlphaTab css_filepath;
};

static
  void
init_HtmlState (HtmlState* st, OFile* ofile)
{
  st->allgood = true;
  st->nlines = 1;
  st->eol = true;
  st->inparagraph = false;
  st->end_document = false;
  st->list_depth = 0;
  st->list_item_open = false;
  st->cram = false;
  st->show_toc = false;
  st->toc_pos = 0;
  st->ofile = ofile;
  st->nsections = 0;
  st->nsubsections = 0;
  init_OFile (st->title);
  init_OFile (st->author);
  init_OFile (st->date);
  init_OFile (st->toc_ofile);
  init_OFile (st->body_ofile);
  InitTable( st->search_paths );
  InitAssocia( AlphaTab, AlphaTab, st->macro_map, cmp_AlphaTab );
  st->css_filepath = dflt_AlphaTab ();
}

static
  void
lose_HtmlState (HtmlState* st)
{
  lose_OFile (st->title);
  lose_OFile (st->author);
  lose_OFile (st->date);
  lose_OFile (st->toc_ofile);
  lose_OFile (st->body_ofile);
  for (i ; st->search_paths.sz)
    lose_AlphaTab (&st->search_paths.s[i]);
  LoseTable( st->search_paths );

  for (Assoc* item = beg_Associa (&st->macro_map); item; )
  {
    AlphaTab* key = (AlphaTab*) key_of_Assoc (&st->macro_map, item);
    AlphaTab* val = (AlphaTab*) val_of_Assoc (&st->macro_map, item);
    Assoc* tmp = item;
    item = next_Assoc (item);

    give_Associa (&st->macro_map, tmp);
    lose_AlphaTab (key);
    lose_AlphaTab (val);
  }
  lose_Associa (&st->macro_map);
  lose_AlphaTab (&st->css_filepath);
}

#define W(s)  oput_cstr_OFile (ofile, s)
static
  void
css_html (HtmlState* st)
{
  OFile* ofile = st->ofile;
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
  W("\ntable {");
  W("\n  border-spacing: 0;");
  W("\n  border-collapse: collapse;");
  W("\n}");
  W("\ntd {");
  W("\n  padding: 0.3em;");
  W("\n  text-align: left;");
  W("\n}");
  W("\n.ljust { text-align: left; }");
  W("\n.cjust { text-align: center; }");
  W("\n.rjust { text-align: right; }");
  W("\ntd.lline { border-left: thin solid black; }");
  W("\ntd.rline { border-right: thin solid black; }");
  W("\ntr.hline { border-top: thin solid black; }");
}

static
  void
head_html (HtmlState* st)
{
  OFile* ofile = st->ofile;
  //W("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
  //W("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML Basic 1.0//EN\" \"http://www.w3.org/TR/xhtml-basic/xhtml-basic10.dtd\">");
  W("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML-Print 1.0//EN\" \"http://www.w3.org/MarkUp/DTD/xhtml-print10.dtd\">");
  W("\n<html xmlns=\"http://www.w3.org/1999/xhtml\">");
  W("\n<head>");
  W("\n<meta http-equiv='Content-Type' content='text/html;charset=utf-8'/>");

  if (empty_ck_AlphaTab (&st->css_filepath)) {
    //W("<style media=\"screen\" type=\"text/css\">\n");
    W("\n<style type=\"text/css\">");
    css_html (st);
    W("\n</style>");
  }
  else {
    W("\n<link rel=\"stylesheet\" type=\"text/css\" href=\"");
    W(ccstr_of_AlphaTab (&st->css_filepath));
    W("\">");
  }

  W("\n<title>"); oput_OFile (ofile, st->title); W("</title>");
  W("\n</head>");
  W("\n<body>");

  W("\n<div class=\"cjust\">");
  W("\n<h1>"); oput_OFile (ofile, st->title); W("</h1>");
  W("\n<h3>"); oput_OFile (ofile, st->author); W("</h3>");
  W("\n<h3>"); oput_OFile (ofile, st->date); W("</h3>");
  W("\n</div>");
}

static
  void
foot_html (HtmlState* st)
{
  OFile* ofile = st->ofile;
  AlphaTab ab = window2_OFile (st->body_ofile, 0, st->toc_pos);
  oput_AlphaTab (st->ofile, &ab);

  if (st->show_toc) {
    oput_cstr_OFile (ofile, "<h3>Contents</h3>");
    oput_OFile (ofile, st->toc_ofile);
    if (st->nsubsections > 0)
      oput_cstr_OFile (ofile, "</li></ol>");
    oput_cstr_OFile (ofile, "</li></ol>");
  }

  ab = window2_OFile (st->body_ofile, st->toc_pos, st->body_ofile->off);
  oput_AlphaTab (st->ofile, &ab);

  //W("<p><a href='http://validator.w3.org/check?uri=referer'>Valid XHTML-Print 1.0</a></p>\n");
  W("\n</body>");
  W("\n</html>\n");
}
#undef W

static void
escape_for_html (OFile* of, XFile* xf, Associa* macro_map);

static void
handle_macro (OFile* of, XFile* xf, Associa* macro_map)
{
  static const char macro_delims[] = "{}()[]\\/. \n\t_";
  char* pos = tods_XFile (xf, macro_delims);
  const char* sym_cstr = ccstr_of_XFile (xf);
  char match = pos[0];
  AlphaTab sym[1];
  Assoc* macro_item;

  if (sym_cstr == pos) {
    switch (match) {
    case '\\':
      oput_char_OFile (of, '\n');
      break;
    case '_':
    case '%':
      oput_char_OFile (of, match);
      break;
    default:
      DBog1( "I don't yet understand: \\%c", match );
      break;
    }

    if (match)
      skipto_XFile (xf, &pos[1]);
    return;
  }
  pos[0] = '\0';
  *sym = dflt1_AlphaTab (sym_cstr);

  macro_item = lookup_Associa (macro_map, sym);
  if (macro_item) {
    XFile olay[1];
    AlphaTab* val = (AlphaTab*) val_of_Assoc (macro_map, macro_item);
    init_XFile_olay_AlphaTab (olay, val);
    escape_for_html (of, olay, macro_map);
  }
  else {
    DBog1( "I don't yet understand: \\%s", sym_cstr );
  }

  pos[0] = match;
  skipto_XFile (xf, pos);
  if (match == '{') {
    nextds_XFile (xf, 0, "}");
  }
}

  void
escape_for_html (OFile* of, XFile* xf, Associa* macro_map)
{
  //const char delims[] = "\"'&<>";
  const char delims[] = "\\\"&<>";
  char* pos;
  while ((pos = tods_XFile (xf, delims)))
  {
    char match = pos[0];

    pos[0] = '\0';
    oput_cstr_OFile (of, ccstr_of_XFile (xf));
    if (!match)  break;

    pos[0] = match;
    pos = &pos[1];
    skipto_XFile (xf, pos);

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
    case '\\':
      if (macro_map)
        handle_macro (of, xf, macro_map);
      else
        oput_char_OFile (of, '\\');
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
        escape_for_html (st->title, olay, &st->macro_map);
      }
    }
    else if (skip_cstr_XFile (xf, "author{"))
    {
      DoLegitLine( "no closing brace" )
        getlined_olay_XFile (olay, xf, "}");
      if (good) {
        escape_for_html (st->author, olay, &st->macro_map);
      }
    }
    else if (skip_cstr_XFile (xf, "date{"))
    {
      DoLegitLine( "no closing brace" )
        getlined_olay_XFile (olay, xf, "}");
      if (good) {
        escape_for_html (st->date, olay, &st->macro_map);
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
  OFile* ofile = st->body_ofile;
  if (st->inparagraph)  return;
  oput_cstr_OFile (ofile, "\n<p");
  if (st->cram) {
    oput_cstr_OFile (ofile, " class=\"cram\"");
  }
  oput_cstr_OFile (ofile, ">");
  st->inparagraph = true;
  st->eol = false;
  st->cram = true;
}

static
  void
close_paragraph (HtmlState* st)
{
  OFile* ofile = st->body_ofile;
  if (!st->inparagraph)  return;
  oput_cstr_OFile (ofile, "</p>");
  st->inparagraph = false;
  st->cram = false;
}

static
  void
open_list (HtmlState* st, const char* tag)
{
  OFile* ofile = st->body_ofile;
  bool cram = st->inparagraph && st->list_depth == 0;
  if (cram) {
    close_paragraph (st);
  }
  st->inparagraph = true;

  st->list_depth += 1;
  oput_cstr_OFile (ofile, "<");
  oput_cstr_OFile (ofile, tag);
  if (cram) {
    oput_cstr_OFile (ofile, " class=\"cram\"");
  }
  oput_cstr_OFile (ofile, ">");
  st->list_item_open = false;
}

static
  void
close_list (HtmlState* st, const char* tag)
{
  OFile* ofile = st->body_ofile;
  if (st->list_item_open) {
    oput_cstr_OFile (ofile, "</li>\n");
  }
  oput_cstr_OFile (ofile, "</");
  oput_cstr_OFile (ofile, tag);
  oput_cstr_OFile (ofile, ">");
  st->list_depth -= 1;
  if (st->list_depth > 0) {
    oput_cstr_OFile (ofile, "</li>\n");
  }
  else {
    st->inparagraph = false;
  }
  st->list_item_open = false;
}

static
  void
next_section (HtmlState* st, XFile* xf, XFile* olay)
{
  OFile* of = st->body_ofile;
  OFile* toc = st->toc_ofile;
  const Trit mayflush = mayflush_XFile (xf, Nil);
  AlphaTab label = dflt_AlphaTab ();
  bool subsec = (st->nsubsections > 0);
  const char* heading = (subsec ? "h4" : "h3");

  skipds_XFile (xf, WhiteSpaceChars);
  if (skip_cstr_XFile (xf, "\\label{")) {
    char* txt = getlined_XFile (xf, "}");
    if (txt)
      label = dflt1_AlphaTab (txt);
  }
  if (empty_ck_AlphaTab (&label)) {
    cat_cstr_AlphaTab (&label, "sec:");
    cat_uint_AlphaTab (&label, st->nsections);
    if (subsec) {
      cat_char_AlphaTab (&label, '.');
      cat_uint_AlphaTab (&label, st->nsubsections);
    }
  }

  if (st->nsections == 1 && st->nsubsections == 0)
    oput_cstr_OFile (st->toc_ofile, "\n<ol class=\"cram\">");
  else if (st->nsubsections == 1)
    oput_cstr_OFile (st->toc_ofile, "\n<ol>");
  else
    oput_cstr_OFile (toc, "</li>");

  oput_cstr_OFile (toc, "\n<li><a href=\"#");
  oput_AlphaTab (toc, &label);
  oput_cstr_OFile (toc, "\">");
  escape_for_html (toc, olay, &st->macro_map);
  oput_cstr_OFile (toc, "</a>");


  printf_OFile (of, "\n<%s id=\"", heading);
  oput_AlphaTab (of, &label);
  lose_AlphaTab (&label);

  printf_OFile (of, "\">%u.", st->nsections);
  if (subsec)
    printf_OFile (of, "%u.", st->nsubsections);
  oput_char_OFile (of, ' ');
  escape_for_html (of, olay, &st->macro_map);
  printf_OFile (of, "</%s>", heading);
  mayflush_XFile (xf, mayflush);
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
// \begin{center}  -->  <div class="cjust">...</div>
// \begin{tabular}  -->  <table>...</table>
// \href{URL}{TEXT}  -->  <a href='URL'>TEXT</a>
// \url{URL}  -->  <a href='URL'>URL</a>
// \section{TEXT}  -->  <h3>TEXT</h3>
// \subsection{TEXT}  -->  <h4>TEXT</h4>
// \label{myname}  -->  <a name="myname">...</a>
static
  bool
htbody (HtmlState* st, XFile* xf, const char* pathname)
{
  DeclLegit( good );
  OFile* of = st->body_ofile;

  while (good && !st->end_document)
  {
    XFile olay[1];
    char match = 0;
    bool was_eol;
    if (!nextds_olay_XFile (olay, xf, &match, "\n\\%$-"))
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
      escape_for_html (of, olay, &st->macro_map);
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
        escape_for_html (of, olay, &st->macro_map);
        oput_cstr_OFile (of, "</i>");
      }
    }
    else if (match == '-') {
      if (skip_cstr_XFile (xf, "-"))
        oput_cstr_OFile (of, "&ndash;");
      else
        oput_char_OFile (of, '-');
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
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, ".</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "expten{")) {
        open_paragraph (st);
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_cstr_OFile (of, "&times;10<sup>");
          escape_for_html (of, olay, &st->macro_map);
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
          //escape_for_html (of, olay, &st->macro_map);
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
          //escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "texttt{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, " <span class=\"texttt\">");
        DoLegitLine( "no closing brace" )
          good = getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
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
          //escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "</span>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilcode{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<code>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, 0);
          oput_cstr_OFile (of, "</code>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilflag{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilfile{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<i>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "</i>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilsym{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "illit{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<i>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "</i>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilname{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ilkey{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<b>");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "</b>");
        }
      }
      else if (skip_cstr_XFile (xf, "ttvbl{")) {
        open_paragraph (st);
        oput_cstr_OFile (of, " <span class=\"ttvbl\">");
        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
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
          escape_for_html (of, olay, 0);
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
          escape_for_html (of, &xfileb->xf, 0);
        oput_cstr_OFile (of, "</code></pre>");
        lose_XFileB (xfileb);
      }
      else if (skip_cstr_XFile (xf, "begin{flushleft}")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<div class=\"ljust\">");
        DoLegitLine( "No end to flushleft!" )
          getlined_olay_XFile (olay, xf, "\\end{flushleft}");
        if (good) {
          htbody (st, olay, pathname);
          oput_cstr_OFile (of, "</div>");
        }
      }
      else if (skip_cstr_XFile (xf, "begin{center}")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<div class=\"cjust\">");
        DoLegitLine( "No end to center!" )
          getlined_olay_XFile (olay, xf, "\\end{center}");
        if (good) {
          htbody (st, olay, pathname);
          oput_cstr_OFile (of, "</div>");
        }
      }
      else if (skip_cstr_XFile (xf, "begin{flushright}")) {
        open_paragraph (st);
        oput_cstr_OFile (of, "<div class=\"rjust\">");
        DoLegitLine( "No end to flushright!" )
          getlined_olay_XFile (olay, xf, "\\end{flushright}");
        if (good) {
          htbody (st, olay, pathname);
          oput_cstr_OFile (of, "</div>");
        }
      }
      else if (skip_cstr_XFile (xf, "begin{tabular}{")) {
        const char* cols = 0;
        const bool inparagraph = st->inparagraph;
        DoLegitLine( "Need \\end{tabular} for \\begin{tabular}!" )
          getlined_olay_XFile (olay, xf, "\n\\end{tabular}");

        DoLegitLineP( cols, "Need column spec for tabular!" )
          getlined_XFile (olay, "}");

        DoLegit( 0 ) {
          uint i;
          XFile line_olay[1];

          oput_cstr_OFile (of, "\n<table>");

          while (getlined_olay_XFile (line_olay, olay, "\\\\")) {
            XFile cell_olay[1];
            i = 0;
            skipds_XFile (line_olay, 0);
            if (skip_cstr_XFile (line_olay, "\\hline"))
              oput_cstr_OFile (of, "\n<tr class=\"hline\">");
            else
              oput_cstr_OFile (of, "\n<tr>");

            while (getlined_olay_XFile (cell_olay, line_olay, "&")) {
              bool lline = false;
              bool rline = false;
              Sign align = -1;

              skipds_XFile (cell_olay, 0);

              if (cols[i]=='|') { ++i; lline = true; }
              if      (cols[i]=='l') { ++i; align = -1; }
              else if (cols[i]=='c') { ++i; align =  0; }
              else if (cols[i]=='r') { ++i; align =  1; }
              if (cols[i]=='|') { ++i; rline = true; }

              if (!lline && !rline && align < 0) {
                oput_cstr_OFile (of, "\n<td>");
              }
              else {
                const char* pfx = "";
                oput_cstr_OFile (of, "\n<td class=\"");
                if (lline) {
                  oput_cstr_OFile (of, pfx);
                  pfx = " ";
                  oput_cstr_OFile (of, "lline");
                }

                if (align == 0) {
                  oput_cstr_OFile (of, pfx);
                  pfx = " ";
                  oput_cstr_OFile (of, "cjust");
                }

                if (align > 0) {
                  oput_cstr_OFile (of, pfx);
                  pfx = " ";
                  oput_cstr_OFile (of, "rjust");
                }

                if (rline) {
                  oput_cstr_OFile (of, pfx);
                  pfx = " ";
                  oput_cstr_OFile (of, "rline");
                }
                oput_cstr_OFile (of, "\">");
              }
              st->inparagraph = true;
              htbody (st, cell_olay, pathname);
              oput_cstr_OFile (of, "</td>");
            }
            oput_cstr_OFile (of, "\n</tr>");
          }
          oput_cstr_OFile (of, "\n</table>");
          st->inparagraph = inparagraph;
        }
      }
      else if (skip_cstr_XFile (xf, "tableofcontents")) {
        st->show_toc = true;
        st->toc_pos = st->body_ofile->off;
      }
      else if (skip_cstr_XFile (xf, "section{")) {
        close_paragraph (st);
        if (st->nsubsections > 0)
          oput_cstr_OFile (st->toc_ofile, "</li></ol>");
        ++ st->nsections;
        st->nsubsections = 0;

        DoLegitLine( "no closing brace for \\section" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          next_section (st, xf, olay);
        }
      }
      else if (skip_cstr_XFile (xf, "subsection{")) {
        close_paragraph (st);
        ++ st->nsubsections;

        DoLegitLine( "no closing brace" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          next_section (st, xf, olay);
        }
      }
      else if (skip_cstr_XFile (xf, "label{")) {
        DoLegitLine( "no closing brace for \\label" )
          getlined_olay_XFile (olay, xf, "}");
        if (good) {
          oput_uint_OFile (of, st->nsections);
          oput_cstr_OFile (of, "<a name=\"");
          oput_cstr_OFile (of, ccstr_of_XFile (olay));
          oput_cstr_OFile (of, "\"></a>");
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
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "'>");
          good = getlined_olay_XFile (olay, xf, "}");
        }
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
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
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "'>");
          escape_for_html (of, olay2, &st->macro_map);
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
          escape_for_html (of, olay, &st->macro_map);
          good = getlined_olay_XFile (olay, xf, "}");
        }
        if (good) {
          escape_for_html (of, olay, &st->macro_map);
          oput_cstr_OFile (of, "'>");
          escape_for_html (of, olay, &st->macro_map);
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
        handle_macro (of, xf, &st->macro_map);
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
    const char* arg = argv[argi++];
    if (eq_cstr ("-x", arg)) {
      DoLegitLine( "open file for reading" )
        open_FileB (&xfb->fb, 0, argv[argi++]);
      if (good) {
        xf = &xfb->xf;
      }
    }
    else if (eq_cstr ("-o", arg)) {
      DoLegitLine( "open file for writing" )
        open_FileB (&ofb->fb, 0, argv[argi++]);
      if (good) {
        st->ofile = &ofb->of;
      }
    }
    else if (eq_cstr ("-o-css", arg)) {
      if (argi == argc) {
        failout_sysCx ("no argument given for -o-css");
      }
      arg = argv[argi++];
      if (!eq_cstr ("-", arg)) {
        DoLegitLine( "open file for writing" )
          open_FileB (&ofb->fb, 0, arg);
        if (good)
          st->ofile = &ofb->of;
      }
      if (good) {
        css_html (st);
      }

      lose_HtmlState (st);
      lose_XFileB (xfb);
      lose_OFileB (ofb);
      lose_sysCx ();
      return (good ? 0 : 1);
    }
    else if (eq_cstr ("-I", arg)) {
      AlphaTab* path = Grow1Table( st->search_paths );
      *path = dflt_AlphaTab ();
      if (argi == argc) {
        failout_sysCx ("no argument given for -I");
      }
      cat_cstr_AlphaTab (path, argv[argi++]);
    }
    else if (eq_cstr ("-css", arg)) {
      if (argi == argc) {
        failout_sysCx ("no argument given for -css");
      }
      arg = argv[argi];
      if (arg) {
        copy_cstr_AlphaTab (&st->css_filepath, arg);
        ++ argi;
      }
    }
    else if (eq_cstr ("-def", arg)) {
      AlphaTab key[1];
      AlphaTab val[1];
      Assoc* item;
      bool added = false;
      if (argi+1 >= argc) {
        failout_sysCx ("Need 2 arguments for -def");
      }

      *key = cons1_AlphaTab (argv[argi++]);
      *val = cons1_AlphaTab (argv[argi++]);

      item = ensure1_Associa (&st->macro_map, key, &added);

      {
        XFile xtmp[1];
        OFile otmp[1];
        init_XFile_olay_AlphaTab (xtmp, val);
        init_OFile (otmp);
        escape_for_html (otmp, xtmp, &st->macro_map);
        copy_AlphaTab_OFile (val, otmp);
        lose_OFile (otmp);
      }

      if (added) {
        val_fo_Assoc (&st->macro_map, item, val);
      }
      else {
        lose_AlphaTab (key);
        copy_AlphaTab ((AlphaTab*) val_of_Assoc (&st->macro_map, item), val);
        lose_AlphaTab (val);
      }
    }
    else {
      good = false;
    }
  }
  if (!good)
    return 1;

  hthead (st, xf);
  htbody (st, xf, ccstr_of_AlphaTab (&xfb->fb.pathname));
  foot_html (st);
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

