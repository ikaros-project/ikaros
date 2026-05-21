const breadcrumbs =
{
    init()
    {
        breadcrumbs.strip = breadcrumbs.getActiveStrip();
    },

    getActiveStrip()
    {
        const pane = document.querySelector(".main_pane.active");
        return pane ? pane.querySelector(".main_breadcrumb_strip") : null;
    },

    buildHTML(item, pane=null)
    {
        let path = "";
        let sep = "";
        let h = "<button class='pane_navigator_toggle_button' onclick='nav.toggleFromButton(this)' title='Show navigator'>&#8801;</button>";
        h += "<span class='breadcrumb_path'>";
        const groups = item.split('.');
        groups.forEach((g, index) =>
        {
            path += sep + g;
            sep = ".";
            if(index > 0)
                h += "<span class='breadcrumb_separator'>&rsaquo;</span>";
            const currentClass = index === groups.length - 1 ? " current" : "";
            h += "<span class='breadcrumb_text" + currentClass + "' onclick='selector.selectItems([], \"" + path + "\")'>" + g + "</span>";
        });
        h += "</span>";
        const canClose = document.querySelectorAll(".main_pane").length > 1;
        const closeDisabled = canClose ? "" : " disabled";
        const inMirror = pane && main && typeof main.isMirrorPane === "function" && main.isMirrorPane(pane);
        const splitDisabled = inMirror ? " disabled" : "";
        h += "<span class='breadcrumb_split_controls'><button onclick='main.splitHorizontal(this)' title='Split horizontally'" + splitDisabled + ">H</button><button onclick='main.splitVertical(this)' title='Split vertically'" + splitDisabled + ">V</button><button onclick='main.splitMirror(this)' title='Mirror split'" + splitDisabled + ">M</button><button onclick='main.closePane(this)' title='Close pane'" + closeDisabled + ">&times;</button></span>";
        return h;
    },

    selectItem(item, pane=null)
    {
        const strip = pane ? pane.querySelector(".main_breadcrumb_strip") : breadcrumbs.getActiveStrip();
        if(strip)
            strip.innerHTML = breadcrumbs.buildHTML(item, pane || document.querySelector(".main_pane.active"));

        breadcrumbs.strip = breadcrumbs.getActiveStrip();
    }
};
