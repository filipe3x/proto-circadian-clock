#!/usr/bin/env python3
"""
Generate professional PDF from commercialization strategy markdown
"""

import markdown
from weasyprint import HTML, CSS
from pathlib import Path

# Read the markdown file
md_content = Path('COMMERCIALIZATION_STRATEGY.md').read_text(encoding='utf-8')

# Convert markdown to HTML
md_extensions = ['tables', 'fenced_code', 'codehilite', 'toc']
html_body = markdown.markdown(md_content, extensions=md_extensions)

# Professional CSS styling
css = """
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap');

@page {
    size: A4;
    margin: 2cm 2.5cm;
    @top-center {
        content: "Proto Circadian Clock - Estratégia de Comercialização";
        font-size: 9pt;
        color: #666;
        font-family: 'Inter', sans-serif;
    }
    @bottom-center {
        content: counter(page) " / " counter(pages);
        font-size: 9pt;
        color: #666;
        font-family: 'Inter', sans-serif;
    }
}

@page :first {
    @top-center { content: none; }
}

body {
    font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    font-size: 10pt;
    line-height: 1.6;
    color: #1a1a1a;
    max-width: 100%;
}

h1 {
    font-size: 24pt;
    font-weight: 700;
    color: #0f172a;
    border-bottom: 3px solid #3b82f6;
    padding-bottom: 12px;
    margin-top: 0;
    margin-bottom: 24px;
    page-break-after: avoid;
}

h2 {
    font-size: 16pt;
    font-weight: 600;
    color: #1e40af;
    margin-top: 28px;
    margin-bottom: 14px;
    padding-bottom: 6px;
    border-bottom: 1px solid #dbeafe;
    page-break-after: avoid;
}

h3 {
    font-size: 12pt;
    font-weight: 600;
    color: #1e3a5f;
    margin-top: 20px;
    margin-bottom: 10px;
    page-break-after: avoid;
}

h4 {
    font-size: 11pt;
    font-weight: 600;
    color: #334155;
    margin-top: 16px;
    margin-bottom: 8px;
}

p {
    margin-bottom: 10px;
    text-align: justify;
}

blockquote {
    background: linear-gradient(135deg, #eff6ff 0%, #dbeafe 100%);
    border-left: 4px solid #3b82f6;
    padding: 16px 20px;
    margin: 20px 0;
    font-style: italic;
    font-size: 11pt;
    color: #1e40af;
    border-radius: 0 8px 8px 0;
}

table {
    width: 100%;
    border-collapse: collapse;
    margin: 16px 0;
    font-size: 9pt;
    page-break-inside: avoid;
}

thead {
    background: linear-gradient(135deg, #1e40af 0%, #3b82f6 100%);
    color: white;
}

th {
    padding: 10px 12px;
    text-align: left;
    font-weight: 600;
    font-size: 9pt;
}

td {
    padding: 8px 12px;
    border-bottom: 1px solid #e2e8f0;
    vertical-align: top;
}

tr:nth-child(even) {
    background-color: #f8fafc;
}

tr:hover {
    background-color: #f1f5f9;
}

code {
    background-color: #f1f5f9;
    padding: 2px 6px;
    border-radius: 4px;
    font-family: 'JetBrains Mono', 'Fira Code', monospace;
    font-size: 9pt;
    color: #0f172a;
}

pre {
    background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%);
    color: #e2e8f0;
    padding: 16px 20px;
    border-radius: 8px;
    overflow-x: auto;
    font-size: 9pt;
    line-height: 1.5;
    margin: 16px 0;
    page-break-inside: avoid;
}

pre code {
    background: none;
    padding: 0;
    color: #e2e8f0;
}

ul, ol {
    margin: 12px 0;
    padding-left: 24px;
}

li {
    margin-bottom: 6px;
}

strong {
    font-weight: 600;
    color: #0f172a;
}

hr {
    border: none;
    height: 2px;
    background: linear-gradient(90deg, #3b82f6 0%, #dbeafe 50%, #3b82f6 100%);
    margin: 32px 0;
}

/* Emoji styling for ratings */
td:last-child, th:last-child {
    text-align: center;
}

/* Cover page styling */
h1:first-of-type {
    font-size: 28pt;
    text-align: center;
    margin-top: 80px;
    margin-bottom: 40px;
    border-bottom: none;
    padding-bottom: 0;
}

h1:first-of-type + h2:first-of-type,
h1:first-of-type ~ p:first-of-type {
    text-align: center;
}

/* Checkboxes */
li input[type="checkbox"] {
    margin-right: 8px;
}

/* Links */
a {
    color: #2563eb;
    text-decoration: none;
}

a:hover {
    text-decoration: underline;
}

/* Footer info */
em {
    color: #64748b;
    font-size: 9pt;
}

/* Section breaks */
h2 {
    page-break-before: auto;
}

/* Ensure tables don't break awkwardly */
table, tr, td, th {
    page-break-inside: avoid;
}
"""

# Complete HTML document
html_doc = f"""
<!DOCTYPE html>
<html lang="pt">
<head>
    <meta charset="UTF-8">
    <title>Proto Circadian Clock - Estratégia de Comercialização</title>
</head>
<body>
{html_body}
</body>
</html>
"""

# Generate PDF
print("Generating PDF...")
html = HTML(string=html_doc)
css_style = CSS(string=css)
html.write_pdf('COMMERCIALIZATION_STRATEGY.pdf', stylesheets=[css_style])

print("✓ PDF generated successfully: COMMERCIALIZATION_STRATEGY.pdf")
