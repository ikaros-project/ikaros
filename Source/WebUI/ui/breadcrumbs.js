const breadcrumbs =
{
    init()
    {
        breadcrumbs.main_breadcrumbs = document.querySelector("#main_breadcrumbs");
    },

    selectItem(item)
    {
        let path = "";
        let sep = "";
        let h = "";
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

        if(breadcrumbs.main_breadcrumbs)
            breadcrumbs.main_breadcrumbs.innerHTML = h;
    }
};
