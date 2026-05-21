const nav =
{
    init()
    {
        nav.navigator = document.getElementById('navigator');
        nav.content = document.getElementById('navigator_tree');
        nav.button = document.getElementById('navigator_toggle_button');
        if(nav.navigator)
            nav.navigator.addEventListener('click', nav.handleClick, false);
    },

    positionAtButton(button)
    {
        if(!nav.navigator || !button)
            return;
        const r = button.getBoundingClientRect();
        nav.navigator.style.left = `${Math.round(r.left)}px`;
        nav.navigator.style.top = `${Math.round(r.top)}px`;
    },

    toggleFromButton(button)
    {
        nav.positionAtButton(button);
        if(button && main && typeof main.getPaneFromSource === "function")
        {
            const pane = main.getPaneFromSource(button);
            if(pane)
            {
                nav.target_pane = pane;
                main.activatePane(pane, false);
            }
        }
        nav.toggle();
    },

    toggle()
    {
        if(nav.navigator.classList.contains("open"))
        {
            nav.navigator.classList.add("closing");
            window.setTimeout(() =>
            {
                nav.navigator.classList.remove("open", "closing");
                if(nav.button)
                    nav.button.innerHTML = "&#8801;";
            }, 140);
            return;
        }

        nav.navigator.classList.add("open");
        if(nav.button)
            nav.button.innerHTML = "&times;";
        nav.revealSelection();
        nav.resizeToContent();
    },

    toggleGroup(e)
    {
        if(e.target.classList.contains("group-open"))
            e.target.classList.replace("group-open", "group-closed");
        else if(e.target.classList.contains("group-closed"))
            e.target.classList.replace("group-closed", "group-open");

        nav.resizeToContent();
        e.stopPropagation();
    },

    openGroup(item)
    {
        let g = nav.content.querySelector("[data-name='" + item + "']");
        if(!g)
            return;
        g = g.parentElement;
        while(g)
        {
            g.classList.remove("group-closed");
            g.classList.add("group-open");
            g = g.parentElement;
        }
        nav.resizeToContent();
    },

    selectItem(item)
    {
        nav.selected_item = item;
        nav.traverseAndSelect(nav.content, item);
        nav.traverseAndOpen(nav.content, item);
    },

    selectModule(evt)
    {
    },

    navClick(e)
    {
        const bg = e.target.parentElement.dataset.name;
        if(nav.target_pane && main && typeof main.activatePane === "function")
            main.activatePane(nav.target_pane, false);
        selector.selectItems([], bg);
        e.stopPropagation();
    },

    handleClick(e)
    {
        const label = e.target.closest("span");
        if(label && nav.content.contains(label))
        {
            nav.navClick(e);
            return;
        }

        const groupItem = e.target.closest("li[data-name]");
        if(groupItem && nav.content.contains(groupItem) && !groupItem.classList.contains("group-empty"))
            nav.toggleGroup({target: groupItem, stopPropagation: () => e.stopPropagation()});
    },

    buildList(group, name)
    {
        if(isEmpty(group))
            return "";

        let fullName = name ? `${name}.${group.name}` : group.name;

        if(group.groups.length == 0)
            return `<li data-name='${fullName}' class='group-empty'><span>${group.name}</span></li>`;

        let s = `<li data-name='${fullName}' class='group-closed'><span>${group.name}</span>`;
        s += `<ul>${group.groups.map((subGroup) => nav.buildList(subGroup, fullName)).join('')}</ul>`;
        s += "</li>";
        return s;
    },

    traverseAndSelect(element, data_name)
    {
        if(!element)
            return;
        if(element.dataset.name == data_name)
            element.classList.add("selected");
        else
            element.classList.remove("selected");
        if(element.children)
            Array.from(element.children).forEach((child) => { nav.traverseAndSelect(child, data_name); });
    },

    traverseAndOpen(element, data_name)
    {
    },

    revealSelection()
    {
        const selectedItem = nav.selected_item || (selector && selector.selected_background);
        if(!selectedItem || !nav.content)
            return;

        nav.openGroup(selectedItem);
        nav.traverseAndSelect(nav.content, selectedItem);
        window.requestAnimationFrame(() =>
        {
            const selectedElement = nav.content.querySelector("[data-name='" + selectedItem + "']");
            if(selectedElement)
                selectedElement.scrollIntoView({block: "nearest", inline: "nearest"});
        });
    },

    resizeToContent()
    {
        if(!nav.navigator || !nav.content)
            return;

        const rect = nav.navigator.getBoundingClientRect();
        const style = window.getComputedStyle(nav.navigator);
        const borderHeight = parseFloat(style.borderTopWidth) + parseFloat(style.borderBottomWidth);
        const paddingHeight = parseFloat(style.paddingTop) + parseFloat(style.paddingBottom);
        const contentStyle = window.getComputedStyle(nav.content);
        const contentTop = parseFloat(contentStyle.marginTop);
        const maxHeight = Math.max(120, window.innerHeight - rect.top - 12);
        const measuredHeight = Math.ceil(nav.content.scrollHeight + contentTop + paddingHeight + borderHeight);

        nav.navigator.style.setProperty("--navigator-open-height", Math.min(measuredHeight, maxHeight) + "px");
    },

    populate()
    {
        nav.content.innerHTML = "<ul>" + nav.buildList(network.network, "") + "</ul>";
        nav.resizeToContent();
    }
};
