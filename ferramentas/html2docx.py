#!/usr/bin/env python3
# Converte os documentos HTML do projeto em .docx no padrao ABNT.
#
# O HTML e a fonte de verdade: este script apenas reexpressa a mesma estrutura
# (secoes numeradas, figuras/quadros/tabelas com legenda e fonte, blocos de
# codigo) nos recursos nativos do Word, para entrega no PIBIC.
#
# Os SVGs sao rasterizados em 300 dpi com cairosvg. As fontes do sistema
# resolvem "Times New Roman" para Liberation Serif, que e metricamente
# compativel, de modo que o desenho sai com as mesmas proporcoes do HTML.

import html
import os
import re
import sys
from html.parser import HTMLParser

import cairosvg
from docx import Document
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_BREAK
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt, RGBColor

SERIF = "Times New Roman"
MONO = "Courier New"

# Area util: A4 (21 x 29,7) menos margens ABNT (esq 3, dir 2, sup 3, inf 2).
LARGURA_UTIL_CM = 16.0
ALTURA_UTIL_CM = 24.7


# --------------------------------------------------------------------------
# Arvore HTML minima
# --------------------------------------------------------------------------

VAZIOS = {"br", "meta", "img", "hr", "link"}


class No:
    def __init__(self, tag, attrs=None):
        self.tag = tag
        self.attrs = dict(attrs or {})
        self.filhos = []

    @property
    def classes(self):
        return self.attrs.get("class", "").split()

    def tem(self, c):
        return c in self.classes

    def texto(self):
        partes = []
        for f in self.filhos:
            partes.append(f if isinstance(f, str) else f.texto())
        return "".join(partes)

    def achar(self, tag, cls=None):
        for f in self.filhos:
            if isinstance(f, No) and f.tag == tag and (cls is None or f.tem(cls)):
                return f
        return None

    def achar_todos(self, tag):
        saida = []
        for f in self.filhos:
            if isinstance(f, No):
                if f.tag == tag:
                    saida.append(f)
                saida.extend(f.achar_todos(tag))
        return saida


class Arvore(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.raiz = No("root")
        self.pilha = [self.raiz]

    def handle_starttag(self, tag, attrs):
        no = No(tag, attrs)
        self.pilha[-1].filhos.append(no)
        if tag not in VAZIOS:
            self.pilha.append(no)

    def handle_endtag(self, tag):
        for i in range(len(self.pilha) - 1, 0, -1):
            if self.pilha[i].tag == tag:
                del self.pilha[i:]
                return

    def handle_data(self, data):
        self.pilha[-1].filhos.append(data)


# --------------------------------------------------------------------------
# Utilidades de formatacao
# --------------------------------------------------------------------------


def fonte(run, nome=SERIF, tam=12):
    run.font.name = nome
    run.font.size = Pt(tam)
    rpr = run._element.get_or_add_rPr()
    rfonts = rpr.find(qn("w:rFonts"))
    if rfonts is None:
        rfonts = OxmlElement("w:rFonts")
        rpr.append(rfonts)
    for a in ("w:ascii", "w:hAnsi", "w:cs", "w:eastAsia"):
        rfonts.set(qn(a), nome)


def bordas_celula(cel, lados):
    """lados: dict lado -> bool. Desenha 1/2 pt preto onde True."""
    tc_pr = cel._tc.get_or_add_tcPr()
    antigo = tc_pr.find(qn("w:tcBorders"))
    if antigo is not None:
        tc_pr.remove(antigo)
    b = OxmlElement("w:tcBorders")
    for lado in ("top", "left", "bottom", "right"):
        el = OxmlElement(f"w:{lado}")
        if lados.get(lado):
            el.set(qn("w:val"), "single")
            el.set(qn("w:sz"), "4")  # 4 oitavos de ponto = 1/2 pt
            el.set(qn("w:color"), "000000")
        else:
            el.set(qn("w:val"), "nil")
        b.append(el)
    tc_pr.append(b)


def moldura_paragrafo(par):
    """Caixa fechada em volta de um paragrafo (usada nos blocos de codigo)."""
    p_pr = par._p.get_or_add_pPr()
    b = OxmlElement("w:pBdr")
    for lado in ("top", "left", "bottom", "right"):
        el = OxmlElement(f"w:{lado}")
        el.set(qn("w:val"), "single")
        el.set(qn("w:sz"), "4")
        el.set(qn("w:space"), "6")
        el.set(qn("w:color"), "000000")
        b.append(el)
    p_pr.append(b)


def sombreado(cel, cor="F2F2F2"):
    tc_pr = cel._tc.get_or_add_tcPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:val"), "clear")
    shd.set(qn("w:fill"), cor)
    tc_pr.append(shd)


def repetir_cabecalho(linha):
    tr_pr = linha._tr.get_or_add_trPr()
    el = OxmlElement("w:tblHeader")
    el.set(qn("w:val"), "true")
    tr_pr.append(el)


# --------------------------------------------------------------------------
# Conversao
# --------------------------------------------------------------------------


class Conversor:
    def __init__(self, doc, base_dir, prefixo_img):
        self.doc = doc
        self.base_dir = base_dir
        self.prefixo_img = prefixo_img
        self.secao = 0
        self.subsecao = 0
        self.subsubsecao = 0
        self.figura = 0
        self.quadro = 0
        self.tabela = 0
        self.n_img = 0

    # ---- runs inline ----

    def runs(self, par, no, **est):
        for f in no.filhos:
            if isinstance(f, str):
                txt = re.sub(r"\s+", " ", f)
                if not txt:
                    continue
                r = par.add_run(txt)
                r.bold = est.get("bold", False)
                r.italic = est.get("italic", False)
                r.font.subscript = est.get("sub", False)
                r.font.superscript = est.get("sup", False)
                if est.get("cinza"):
                    r.font.color.rgb = RGBColor(0x44, 0x44, 0x44)
                fonte(r, MONO if est.get("mono") else SERIF,
                      11 if est.get("mono") else est.get("tam", 12))
                continue

            novo = dict(est)
            if f.tag in ("em", "i"):
                novo["italic"] = True
            elif f.tag in ("strong", "b"):
                novo["bold"] = True
            elif f.tag == "code":
                novo["mono"] = True
            elif f.tag == "sub":
                novo["sub"] = True
            elif f.tag == "sup":
                novo["sup"] = True
            elif f.tag == "span" and f.tem("lacuna"):
                novo["italic"] = True
                novo["cinza"] = True
            elif f.tag == "br":
                par.add_run().add_break()
                continue
            self.runs(par, f, **novo)

    # ---- paragrafos ----

    def p_corpo(self, no):
        par = self.doc.add_paragraph()
        pf = par.paragraph_format
        pf.line_spacing = 1.5
        pf.space_after = Pt(0)
        centro = "text-align:center" in no.attrs.get("style", "").replace(" ", "")
        if centro:
            pf.alignment = WD_ALIGN_PARAGRAPH.CENTER
            pf.space_before = Pt(6)
            pf.space_after = Pt(6)
        else:
            pf.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
        if not (no.tem("sem-recuo") or centro):
            pf.first_line_indent = Cm(1.25)
        self.runs(par, no)
        return par

    def p_nota(self, no):
        par = self.doc.add_paragraph()
        pf = par.paragraph_format
        pf.line_spacing = 1.0
        pf.space_before = Pt(6)
        pf.space_after = Pt(6)
        pf.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
        self.runs(par, no, tam=10)
        return par

    def titulo(self, no):
        nivel = int(no.tag[1])
        par = self.doc.add_paragraph()
        pf = par.paragraph_format
        pf.line_spacing = 1.5
        pf.space_before = Pt(18)
        pf.space_after = Pt(12)
        pf.alignment = WD_ALIGN_PARAGRAPH.LEFT
        pf.keep_with_next = True

        texto = re.sub(r"\s+", " ", no.texto()).strip()
        if nivel == 1:
            if no.tem("sem-numero"):
                rotulo = ""
                pf.alignment = WD_ALIGN_PARAGRAPH.CENTER
            else:
                self.secao += 1
                self.subsecao = 0
                rotulo = f"{self.secao} "
            r = par.add_run(rotulo + texto.upper())
            r.bold = True
            fonte(r, SERIF, 12)
        elif nivel == 2:
            self.subsecao += 1
            self.subsubsecao = 0
            r = par.add_run(f"{self.secao}.{self.subsecao} ")
            r.bold = True
            fonte(r, SERIF, 12)
            self.runs(par, no, bold=True)
        else:
            self.subsubsecao += 1
            r = par.add_run(
                f"{self.secao}.{self.subsecao}.{self.subsubsecao} ")
            r.italic = True
            fonte(r, SERIF, 12)
            self.runs(par, no, italic=True)
        return par

    def lista(self, no, ordenada=False):
        for i, li in enumerate(f for f in no.filhos
                               if isinstance(f, No) and f.tag == "li"):
            par = self.doc.add_paragraph()
            pf = par.paragraph_format
            pf.line_spacing = 1.5
            pf.space_after = Pt(0)
            pf.left_indent = Cm(1.25)
            pf.first_line_indent = Cm(-0.5)
            pf.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
            marca = f"{i + 1}. " if ordenada else "– "
            r = par.add_run(marca)
            fonte(r, SERIF, 12)
            self.runs(par, li)

    # ---- figuras, quadros, tabelas ----

    def legenda(self, no_legenda, tipo):
        if tipo == "figura":
            self.figura += 1
            rotulo = f"Figura {self.figura} – "
        elif tipo == "quadro":
            self.quadro += 1
            rotulo = f"Quadro {self.quadro} – "
        else:
            self.tabela += 1
            rotulo = f"Tabela {self.tabela} – "

        par = self.doc.add_paragraph()
        pf = par.paragraph_format
        pf.line_spacing = 1.0
        pf.space_before = Pt(12)
        pf.space_after = Pt(4)
        pf.alignment = WD_ALIGN_PARAGRAPH.CENTER
        pf.keep_with_next = True
        r = par.add_run(rotulo)
        fonte(r, SERIF, 10)
        self.runs(par, no_legenda, tam=10)

    def fonte_figura(self, no_fonte):
        par = self.doc.add_paragraph()
        pf = par.paragraph_format
        pf.line_spacing = 1.0
        pf.space_before = Pt(4)
        pf.space_after = Pt(12)
        pf.alignment = WD_ALIGN_PARAGRAPH.CENTER
        self.runs(par, no_fonte, tam=10)

    def desenho(self, svg_txt):
        """Rasteriza o SVG em 300 dpi e insere centralizado."""
        m = re.search(r'viewBox="([\d.\-\s]+)"', svg_txt)
        vb = [float(x) for x in m.group(1).split()] if m else [0, 0, 800, 600]
        larg_u, alt_u = vb[2], vb[3]

        # Escolhe a dimensao limitante para caber na mancha.
        cm_por_u = min(LARGURA_UTIL_CM / larg_u, ALTURA_UTIL_CM / alt_u)
        larg_cm = larg_u * cm_por_u
        alt_cm = alt_u * cm_por_u

        px = int(larg_cm / 2.54 * 300)  # 300 dpi
        self.n_img += 1
        caminho = os.path.join(
            self.base_dir, f"{self.prefixo_img}-{self.n_img:02d}.png")
        cairosvg.svg2png(bytestring=svg_txt.encode("utf-8"),
                         write_to=caminho, output_width=px,
                         background_color="white")

        par = self.doc.add_paragraph()
        par.paragraph_format.alignment = WD_ALIGN_PARAGRAPH.CENTER
        par.paragraph_format.space_after = Pt(0)
        par.paragraph_format.keep_with_next = True
        par.add_run().add_picture(caminho, width=Cm(larg_cm))

    def bloco_codigo(self, no_pre):
        bruto = html.unescape(no_pre.texto())
        linhas = bruto.strip("\n").split("\n")
        par = self.doc.add_paragraph()
        pf = par.paragraph_format
        pf.line_spacing = 1.0
        pf.space_before = Pt(0)
        pf.space_after = Pt(0)
        pf.alignment = WD_ALIGN_PARAGRAPH.LEFT
        pf.keep_with_next = True
        moldura_paragrafo(par)
        for i, linha in enumerate(linhas):
            if i:
                par.add_run().add_break()
            r = par.add_run(linha)
            fonte(r, MONO, 9.5)

    def tabela_word(self, no_table, fechada):
        linhas = no_table.achar_todos("tr")
        n_col = max(len([c for c in tr.filhos
                         if isinstance(c, No) and c.tag in ("td", "th")])
                    for tr in linhas)
        t = self.doc.add_table(rows=0, cols=n_col)
        t.alignment = WD_TABLE_ALIGNMENT.CENTER
        t.autofit = True

        for idx, tr in enumerate(linhas):
            celulas = [c for c in tr.filhos
                       if isinstance(c, No) and c.tag in ("td", "th")]
            linha = t.add_row()
            cabecalho = all(c.tag == "th" for c in celulas)
            if cabecalho:
                repetir_cabecalho(linha)
            for j, c in enumerate(celulas):
                cel = linha.cells[j]
                par = cel.paragraphs[0]
                pf = par.paragraph_format
                pf.line_spacing = 1.0
                pf.space_before = Pt(2)
                pf.space_after = Pt(2)
                if c.tem("num"):
                    pf.alignment = WD_ALIGN_PARAGRAPH.RIGHT
                else:
                    pf.alignment = WD_ALIGN_PARAGRAPH.LEFT
                self.runs(par, c, tam=10, bold=(c.tag == "th"))
                if fechada:
                    bordas_celula(cel, {"top": True, "left": True,
                                        "bottom": True, "right": True})
                else:
                    bordas_celula(cel, {"top": True, "bottom": True,
                                        "left": False, "right": False})
                if cabecalho:
                    sombreado(cel)

    def bloco_figura(self, no):
        if no.tem("figura"):
            tipo = "figura"
        elif no.tem("quadro"):
            tipo = "quadro"
        else:
            tipo = "tabela"

        leg = no.achar("p", "legenda")
        if leg is not None:
            self.legenda(leg, tipo)

        for f in no.filhos:
            if not isinstance(f, No):
                continue
            if f.tag == "div" and f.tem("desenho"):
                svg = f.achar("svg")
                if svg is not None:
                    self.desenho(self.svg_bruto[id(svg)])
            elif f.tag == "pre":
                self.bloco_codigo(f)
            elif f.tag == "table":
                self.tabela_word(f, fechada=(tipo == "quadro"))

        fnt = no.achar("p", "fonte")
        if fnt is not None:
            self.fonte_figura(fnt)

    # ---- sumario (campo TOC nativo do Word) ----

    def sumario_campo(self):
        par = self.doc.add_paragraph()
        pf = par.paragraph_format
        pf.line_spacing = 1.0
        pf.space_after = Pt(6)

        def fld_char(tipo):
            r = par.add_run()
            fonte(r, SERIF, 12)
            el = OxmlElement("w:fldChar")
            el.set(qn("w:fldCharType"), tipo)
            r._r.append(el)

        fld_char("begin")
        r_instr = par.add_run()
        fonte(r_instr, SERIF, 12)
        instr = OxmlElement("w:instrText")
        instr.set(qn("xml:space"), "preserve")
        instr.text = ' TOC \\o "1-3" \\h \\z \\u '
        r_instr._r.append(instr)
        fld_char("separate")
        r_aviso = par.add_run(
            "Sumário gerado automaticamente — no Word ou LibreOffice, clique "
            "com o botão direito sobre esta área e escolha “Atualizar "
            "campo” (ou selecione tudo e pressione F9) para preencher "
            "os títulos e números de página.")
        r_aviso.italic = True
        fonte(r_aviso, SERIF, 11)
        fld_char("end")

    # ---- capa ----

    def rosto(self, no):
        for f in no.filhos:
            if not isinstance(f, No) or f.tag != "p":
                continue
            par = self.doc.add_paragraph()
            pf = par.paragraph_format
            pf.alignment = WD_ALIGN_PARAGRAPH.CENTER
            pf.line_spacing = 1.5
            pf.space_after = Pt(0)
            if f.tem("titulo"):
                pf.space_before = Pt(150)
                pf.space_after = Pt(12)
                self.runs(par, f, bold=True)
                for r in par.runs:
                    r.text = r.text.upper()
            elif f.tem("subtitulo"):
                pf.space_after = Pt(150)
                self.runs(par, f)
            elif f.tem("instituicao"):
                self.runs(par, f, bold=True)
                for r in par.runs:
                    r.text = r.text.upper()
            elif f.tem("local"):
                pf.space_before = Pt(150)
                self.runs(par, f)
            else:
                self.runs(par, f)
        self.doc.add_paragraph().add_run().add_break(WD_BREAK.PAGE)

    # ---- percurso ----

    def percorrer(self, no):
        for f in no.filhos:
            if not isinstance(f, No):
                continue
            t = f.tag
            if t == "div" and f.tem("rosto"):
                self.rosto(f)
            elif t == "div" and f.tem("sumario-campo"):
                self.sumario_campo()
            elif t == "div" and f.tem("referencias"):
                for p in f.filhos:
                    if isinstance(p, No) and p.tag == "p":
                        par = self.doc.add_paragraph()
                        pf = par.paragraph_format
                        pf.line_spacing = 1.0
                        pf.space_after = Pt(12)
                        pf.alignment = WD_ALIGN_PARAGRAPH.LEFT
                        self.runs(par, p)
            elif t in ("h1", "h2", "h3"):
                self.titulo(f)
            elif t == "p":
                if f.tem("nota"):
                    self.p_nota(f)
                else:
                    self.p_corpo(f)
            elif t in ("ul", "ol"):
                self.lista(f, ordenada=(t == "ol"))
            elif t == "figure":
                self.bloco_figura(f)
            elif t in ("style", "title", "meta", "script"):
                continue
            else:
                self.percorrer(f)


def converter(entrada, saida):
    txt = open(entrada, encoding="utf-8").read()
    svgs_brutos = re.findall(r"<svg.*?</svg>", txt, re.S)

    arv = Arvore()
    arv.feed(txt)

    doc = Document()

    sec = doc.sections[0]
    sec.page_width = Cm(21)
    sec.page_height = Cm(29.7)
    sec.top_margin = Cm(3)
    sec.bottom_margin = Cm(2)
    sec.left_margin = Cm(3)
    sec.right_margin = Cm(2)

    normal = doc.styles["Normal"]
    normal.font.name = SERIF
    normal.font.size = Pt(12)
    rpr = normal.element.get_or_add_rPr()
    rfonts = rpr.find(qn("w:rFonts"))
    if rfonts is None:
        rfonts = OxmlElement("w:rFonts")
        rpr.append(rfonts)
    for a in ("w:ascii", "w:hAnsi", "w:cs", "w:eastAsia"):
        rfonts.set(qn(a), SERIF)

    # Faz o Word recalcular o campo TOC (sumario) ao abrir o arquivo, sem
    # exigir F9 manual.
    settings_el = doc.settings.element
    update_fields = OxmlElement("w:updateFields")
    update_fields.set(qn("w:val"), "true")
    settings_el.append(update_fields)

    base = os.path.dirname(os.path.abspath(saida))
    prefixo = os.path.splitext(os.path.basename(saida))[0]
    conv = Conversor(doc, base, prefixo)

    # Casa cada <svg> da arvore com seu texto bruto, na ordem.
    ordem = arv.raiz.achar_todos("svg")
    conv.svg_bruto = {id(n): s for n, s in zip(ordem, svgs_brutos)}

    conv.percorrer(arv.raiz)
    doc.save(saida)

    for i in range(1, conv.n_img + 1):
        p = os.path.join(base, f"{prefixo}-{i:02d}.png")
        if os.path.exists(p):
            os.remove(p)

    return conv


if __name__ == "__main__":
    entrada, saida = sys.argv[1], sys.argv[2]
    c = converter(entrada, saida)
    print(f"{saida}: {c.secao} secoes, {c.figura} figuras, "
          f"{c.quadro} quadros, {c.tabela} tabelas, {c.n_img} desenhos")
