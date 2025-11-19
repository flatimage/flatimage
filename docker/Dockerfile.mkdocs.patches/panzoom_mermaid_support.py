    def on_post_page(self, output: str, /, *, page, config):

        excluded_pages = self.config.get("exclude",[])

        if exclude(page.file.src_path,excluded_pages):
            return

        html_page = re.sub(r"(<\/head>)", f"{create_meta_tags(config)} \\1", output, count=1)

        # Wrap mermaid and d2 diagrams in panzoom boxes
        from mkdocs_panzoom_plugin.panzoom_box import create_panzoom_box
        box_id = 0

        # Wrap mermaid diagrams
        if ".mermaid" in config.get("selectors", []):
            def wrap_mermaid(match):
                nonlocal box_id
                box = create_panzoom_box(self.config, box_id)
                box_id += 1
                return box.replace("\\1", match.group(0))
            html_page = re.sub(r'<pre><div class="mermaid">.*?</div></pre>', wrap_mermaid, html_page, flags=re.DOTALL)

        return str(html_page)
