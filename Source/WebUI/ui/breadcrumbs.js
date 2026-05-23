const breadcrumbs =
{
    splitIcon(direction)
    {
        if(direction === "horizontal")
            return "<svg class='breadcrumb_split_icon' viewBox='0 0 16 16' aria-hidden='true'><rect x='2.5' y='2.5' width='11' height='11' rx='1.5' fill='none' stroke='currentColor' stroke-width='1.4'></rect><path d='M3 8 H13' fill='none' stroke='currentColor' stroke-width='1.4' stroke-linecap='round'></path></svg>";
        if(direction === "vertical")
            return "<svg class='breadcrumb_split_icon' viewBox='0 0 16 16' aria-hidden='true'><rect x='2.5' y='2.5' width='11' height='11' rx='1.5' fill='none' stroke='currentColor' stroke-width='1.4'></rect><path d='M8 3 V13' fill='none' stroke='currentColor' stroke-width='1.4' stroke-linecap='round'></path></svg>";
        if(direction === "mirror")
            return "<svg class='breadcrumb_split_icon' viewBox='0 0 16 16' aria-hidden='true'><path d='M8 3 V13' fill='none' stroke='currentColor' stroke-width='1.2' stroke-linecap='round' stroke-dasharray='1 2'></path><path d='M4.5 5 L1.5 8 L4.5 11 M11.5 5 L14.5 8 L11.5 11' fill='none' stroke='currentColor' stroke-width='1.4' stroke-linecap='round' stroke-linejoin='round'></path></svg>";
        return "";
    },

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
        h += "<span class='breadcrumb_split_controls'><button class='breadcrumb_icon_button' onclick='main.splitHorizontal(this)' title='Split horizontally' aria-label='Split horizontally'" + splitDisabled + ">" + breadcrumbs.splitIcon("horizontal") + "</button><button class='breadcrumb_icon_button' onclick='main.splitVertical(this)' title='Split vertically' aria-label='Split vertically'" + splitDisabled + ">" + breadcrumbs.splitIcon("vertical") + "</button><button class='breadcrumb_icon_button' onclick='main.splitMirror(this)' title='Mirror split' aria-label='Mirror split'" + splitDisabled + ">" + breadcrumbs.splitIcon("mirror") + "</button><button class='breadcrumb_close_button' onclick='main.closePane(this)' title='Close pane'" + closeDisabled + ">&times;</button></span>";
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
