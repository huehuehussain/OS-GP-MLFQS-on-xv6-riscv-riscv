# LaTeX Report Generation Guide

## File Information

**Report File:** `MLFQ_Project_Report.tex`

**Specifications:**
- **Format:** LaTeX/PDF
- **Font:** Times New Roman 11pt
- **Spacing:** 1.15 line spacing (one-half spacing)
- **Citation Style:** IEEE (standard for technical papers)
- **Length:** 5-8 pages (complete report)
- **Content:** Weeks 1-3 comprehensive coverage

---

## Prerequisites

To compile the LaTeX document to PDF, you need:

### Windows
1. **MiKTeX** - Full LaTeX distribution
   - Download: https://miktex.org/download
   - Installation: Run installer, select "Install missing packages on-the-fly"

2. **Optional: TeXstudio** - IDE for LaTeX editing
   - Download: https://www.texstudio.org/

### macOS
```bash
brew install --cask mactex
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install texlive-latex-extra texlive-fonts-recommended
```

---

## Compilation Methods

### Method 1: Command Line (Recommended)

Navigate to project directory and run:

```bash
# Single pass compilation
pdflatex MLFQ_Project_Report.tex

# Complete compilation with bibliography
pdflatex MLFQ_Project_Report.tex
pdflatex MLFQ_Project_Report.tex  # Run twice for references

# Output
# Result: MLFQ_Project_Report.pdf
```

### Method 2: TeXstudio (GUI)

1. Open TeXstudio
2. File → Open → Select `MLFQ_Project_Report.tex`
3. Click "Build & View" (green play button)
4. PDF opens automatically

### Method 3: Online (No Installation)

Use Overleaf (https://www.overleaf.com/):
1. Create new project
2. Upload `MLFQ_Project_Report.tex`
3. Click "Recompile" - instant PDF

---

## Document Structure

### Front Matter
- **Title Page:** Project title, date, authors
- **Abstract:** 150-word summary of entire project
- **Table of Contents:** Automatic section indexing

### Main Sections (6 pages)

1. **Introduction** (0.5 pages)
   - Background on process scheduling
   - Project objectives
   - Architecture overview

2. **Week 1: Foundation** (1 page)
   - Design and architecture (4-level queue system)
   - Data structures (proc fields)
   - getprocinfo() syscall implementation
   - Process lifecycle integration

3. **Week 2: Scheduler** (1.5 pages)
   - Queue operations (6 functions)
   - Main scheduler algorithm with code
   - Time quantum enforcement
   - I/O process fairness mechanism

4. **Week 3: Testing** (1.5 pages)
   - Statistics infrastructure
   - boostproc() and getschedulerstats() syscalls
   - Comprehensive test suite overview
   - Real-time statistics tool

5. **Implementation Analysis** (0.5 pages)
   - Code statistics table
   - Synchronization mechanisms
   - Performance characteristics

6. **Results and Verification** (0.5 pages)
   - CPU-bound process behavior
   - I/O-bound process behavior
   - Starvation prevention evidence

7. **Conclusion** (0.5 pages)
   - Summary of achievements
   - Production quality assessment

### Back Matter
- **References:** IEEE format citations
- **Appendix A:** File modifications table
- **Appendix B:** System call reference
- **Appendix C:** Compilation and execution guide

---

## Output File

**Generated File:** `MLFQ_Project_Report.pdf`

**Properties:**
- Professional formatting with headers/footers
- Page numbers (Page X of Y)
- Syntax-highlighted code listings
- Tables with proper formatting
- Hyperlinked table of contents and references
- IEEE-style bibliography

---

## Customization Options

### Change Author/Date

Edit in the TeX file:
```latex
\author{Your Name}
\date{\today}  % or \date{December 4, 2025}
```

### Adjust Spacing

Current: 1.15 spacing (onehalfspacing)

Options:
```latex
\singlespacing    % 1.0
\onehalfspacing   % 1.15 (current)
\doublespacing    % 2.0
```

### Change Header/Footer

Edit:
```latex
\rhead{Your Header}
\lhead{Your Left Header}
```

### Add More Sections

Template:
```latex
\section{New Section}
\subsection{Subsection}

Content here...

\subsubsection{Sub-subsection}
More content...
```

### Add Figures

```latex
\begin{figure}[h]
  \centering
  \includegraphics[width=0.8\textwidth]{image_path}
  \caption{Caption text}
  \label{fig:label}
\end{figure}

Reference: Figure~\ref{fig:label}
```

### Add Equations

Inline: `$\alpha + \beta = \gamma$`

Display:
```latex
\begin{equation}
  \alpha + \beta = \gamma
  \label{eq:label}
\end{equation}

Reference: Equation~\ref{eq:label}
```

---

## Common Issues & Solutions

### Issue: "Package not found" error

**Solution:** 
```bash
# MiKTeX: Let installer download packages
# Ubuntu: sudo apt-get install texlive-full
```

### Issue: Compilation hangs

**Solution:** Press Ctrl+C and try again, or check for infinite loops in code

### Issue: References not appearing

**Solution:** Run LaTeX compilation twice:
```bash
pdflatex MLFQ_Project_Report.tex
pdflatex MLFQ_Project_Report.tex
```

### Issue: Hyperlinks not colored

**Solution:** Already configured in preamble with `\usepackage{hyperref}`

---

## File Statistics

The generated PDF will be approximately:

| Metric | Value |
|--------|-------|
| Pages | 7-8 |
| Words | ~4,000 |
| Code Listings | 15+ |
| Figures/Tables | 10+ |
| References | 5 |
| File Size | ~800 KB |

---

## IEEE Citation Examples

**Book:**
```latex
\bibitem{author}
Author, A. (Year). ``Book title.'' Publisher.
```

**Journal Article:**
```latex
\bibitem{author}
Author, A., \& Author, B. (Year). ``Article title.'' 
Journal Name, volume(issue), pages.
```

**Webpage:**
```latex
\bibitem{website}
Organization. (Year). ``Title.'' Retrieved from URL
```

---

## Verification Checklist

Before submitting:
- [ ] PDF compiles without errors
- [ ] All pages numbered correctly
- [ ] Table of contents links work
- [ ] Code listings render properly
- [ ] References formatted in IEEE style
- [ ] All figures/tables have captions
- [ ] No spelling/grammar errors
- [ ] Spacing is 1.15 line height
- [ ] Font is Times New Roman 11pt
- [ ] Margins are 1 inch all sides

---

## Quick Start Commands

**One-line compilation (Windows PowerShell):**
```powershell
cd "d:\OS-GP-MLFQS-on-xv6-riscv-riscv" ; pdflatex MLFQ_Project_Report.tex ; pdflatex MLFQ_Project_Report.tex
```

**Linux/Mac:**
```bash
cd ~/OS-GP-MLFQS-on-xv6-riscv-riscv && pdflatex MLFQ_Project_Report.tex && pdflatex MLFQ_Project_Report.tex && open MLFQ_Project_Report.pdf
```

---

## Additional LaTeX Resources

- **Official LaTeX Documentation:** https://www.latex-project.org/
- **Overleaf Tutorials:** https://www.overleaf.com/learn
- **IEEE Citation Guide:** https://www.ieee.org/documents/style_manual.pdf
- **LaTeX WikiBook:** https://en.wikibooks.org/wiki/LaTeX

---

## Support

If compilation fails:

1. Ensure all required packages are installed
2. Check LaTeX syntax in the TeX file
3. Verify file encoding is UTF-8
4. Try online compilation at Overleaf first
5. Check MiKTeX console for missing packages

**Generated:** December 2024  
**LaTeX Version:** ≥ 2020  
**Required Packages:** inputenc, geometry, setspace, times, listings, xcolor, hyperref, fancyhdr
