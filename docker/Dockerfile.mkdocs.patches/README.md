# MkDocs Patches

This directory contains patches applied to MkDocs plugins during the Docker build process.

## panzoom_mermaid_support.py

**Plugin:** mkdocs-panzoom-plugin v0.4.2

**Issue:** The panzoom plugin does not wrap mermaid.

**Solution:** This patch modifies the plugin's `on_post_page` method to wrap mermaid diagrams in panzoom-box divs during HTML post-processing, after mermaid2 has already converted the diagrams.

**What it does:**
- Wraps `<pre><div class="mermaid">...</div></pre>` elements in panzoom-box containers
- Assigns unique IDs to each panzoom box
- Preserves all panzoom configuration (key bindings, hints, etc.)
- Only applies when `.mermaid` is in the configured selectors

**Result:** Users can now pan & zoom on mermaid diagrams by holding the configured modifier key (Shift, Alt, or Ctrl) and using mouse scroll/drag.