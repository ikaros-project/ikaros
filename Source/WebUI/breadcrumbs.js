const breadcrumbs =
{
    init()
    {
        breadcrumbs.breadcrumbs = document.querySelector("#breadcrumbs");
    },

    selectItem(item)
    {
        const crum = breadcrumbs.breadcrumbs.querySelectorAll('.dynamic');
        crum.forEach((crumb) => { crumb.remove(); });
        let path = "";
        let sep = "";
        let h = "";
        for(const g of item.split('.'))
        {
            path += sep + g;
            sep = ".";
            let styleStr = "";
            if(path == item)
            {
                styleStr = "style='--breadcrumb-element-color: var(--breadcrumb-active-color); border-radius: 5px;'";
                h += "<div class='dynamic' " + styleStr + " onclick='selector.selectItems([], \"" + path + "\")'>" + g + "</div>";
            }
            else
            {
                h += "<div class='bread dynamic' " + styleStr + " onclick='selector.selectItems([], \"" + path + "\")'>" + g + "</div>";
            }
        }
        h += "</div>";
        document.querySelector("#nav").insertAdjacentHTML('afterend', h);
    }
};
