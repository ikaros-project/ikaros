const nav =
{
    init()
    {
        nav.selected_item = null;
    },

    createNavigator(pane)
    {
        const navigator = document.createElement("nav");
        navigator.className = "pane_navigator";
        navigator.innerHTML = "<button class='navigator_toggle_button' onclick='nav.toggleFromNavigatorButton(this)'>&#8801;</button><div class='navigator_tree'></div>";
        navigator.addEventListener("click", nav.handleClick, false);
        navigator.target_pane = pane;
        pane.appendChild(navigator);
        nav.populateNavigator(navigator);
        return navigator;
    },

    getNavigator(pane)
    {
        if(!pane)
            return null;
        let navigator = Array.from(pane.children).find((child) => child.classList && child.classList.contains("pane_navigator"));
        if(!navigator)
            navigator = nav.createNavigator(pane);
        return navigator;
    },

    getNavigatorFromSource(source)
    {
        return source && source.closest ? source.closest("nav.pane_navigator") : null;
    },

    getPaneFromNavigator(navigator)
    {
        return navigator && navigator.closest ? navigator.closest(".main_pane") : null;
    },

    getContent(navigator)
    {
        return navigator ? navigator.querySelector(".navigator_tree") : null;
    },

    getButton(navigator)
    {
        return navigator ? navigator.querySelector(".navigator_toggle_button") : null;
    },

    positionAtButton(button)
    {
        if(!button)
            return null;
        const pane = main && typeof main.getPaneFromSource === "function" ? main.getPaneFromSource(button) : button.closest(".main_pane");
        const navigator = nav.getNavigator(pane);
        if(!navigator)
            return null;

        const r = button.getBoundingClientRect();
        const controls = pane ? pane.querySelector(".component_create_controls") : null;
        const firstControlButton = controls ? controls.querySelector("button") : null;
        const controlsRect = firstControlButton ? firstControlButton.getBoundingClientRect() : null;
        navigator.style.left = `${Math.round(r.left)}px`;
        navigator.style.top = `${Math.round((controlsRect ? controlsRect.top : r.top) + 60)}px`;
        return navigator;
    },

    toggleFromButton(button)
    {
        const navigator = nav.positionAtButton(button);
        if(!navigator)
            return;

        const pane = main && typeof main.getPaneFromSource === "function" ? main.getPaneFromSource(button) : null;
        if(pane)
        {
            navigator.target_pane = pane;
            main.activatePane(pane, false);
        }
        nav.toggle(navigator);
    },

    toggleFromNavigatorButton(button)
    {
        const navigator = nav.getNavigatorFromSource(button);
        nav.toggle(navigator);
    },

    setTargetPane(pane)
    {
        const navigator = nav.getNavigator(pane);
        if(navigator)
            navigator.target_pane = pane;
    },

    toggle(navigator)
    {
        if(!navigator)
            return;

        const button = nav.getButton(navigator);
        if(navigator.classList.contains("open"))
        {
            navigator.classList.add("closing");
            window.setTimeout(() =>
            {
                navigator.classList.remove("open", "closing");
                if(button)
                    button.innerHTML = "&#8801;";
            }, 140);
            return;
        }

        navigator.classList.add("open");
        if(button)
            button.innerHTML = "&times;";
        nav.revealSelection(navigator);
        nav.resizeToContent(navigator);
    },

    toggleGroup(e)
    {
        if(e.target.classList.contains("group-open"))
            e.target.classList.replace("group-open", "group-closed");
        else if(e.target.classList.contains("group-closed"))
            e.target.classList.replace("group-closed", "group-open");

        nav.resizeToContent(nav.getNavigatorFromSource(e.target));
        e.stopPropagation();
    },

    openGroup(item, navigator)
    {
        const content = nav.getContent(navigator);
        if(!content)
            return;
        let g = content.querySelector("[data-name='" + item + "']");
        if(!g)
            return;
        g = g.parentElement;
        while(g)
        {
            g.classList.remove("group-closed");
            g.classList.add("group-open");
            g = g.parentElement;
        }
        nav.resizeToContent(navigator);
    },

    selectItem(item, pane=null)
    {
        nav.selected_item = item;
        if(pane)
        {
            nav.selectItemInNavigator(nav.getNavigator(pane), item);
            return;
        }

        document.querySelectorAll("nav.pane_navigator").forEach((navigator) =>
        {
            const paneForNavigator = nav.getPaneFromNavigator(navigator);
            nav.selectItemInNavigator(navigator, paneForNavigator && paneForNavigator.dataset.background ? paneForNavigator.dataset.background : item);
        });
    },

    selectItemInNavigator(navigator, item)
    {
        const content = nav.getContent(navigator);
        if(!content)
            return;
        nav.traverseAndSelect(content, item);
        nav.traverseAndOpen(content, item);
    },

    selectModule(evt)
    {
    },

    navClick(e)
    {
        const navigator = nav.getNavigatorFromSource(e.target);
        const pane = navigator && navigator.target_pane ? navigator.target_pane : nav.getPaneFromNavigator(navigator);
        const item = e.target.parentElement;
        const bg = item ? item.dataset.name : null;
        if(!bg)
            return;
        if(pane && main && typeof main.activatePane === "function")
            main.activatePane(pane, false);
        selector.selectItems([], bg);
        nav.selectItem(bg, pane);
        e.stopPropagation();
    },

    handleClick(e)
    {
        const navigator = nav.getNavigatorFromSource(e.target);
        const content = nav.getContent(navigator);
        if(!content)
            return;

        const label = e.target.closest("span");
        if(label && content.contains(label))
        {
            nav.navClick(e);
            return;
        }

        const groupItem = e.target.closest("li[data-name]");
        if(groupItem && content.contains(groupItem) && !groupItem.classList.contains("group-empty"))
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

    revealSelection(navigator)
    {
        const pane = nav.getPaneFromNavigator(navigator);
        const content = nav.getContent(navigator);
        const selectedItem = (pane && pane.dataset.background) || nav.selected_item || (selector && selector.selected_background);
        if(!selectedItem || !content)
            return;

        nav.openGroup(selectedItem, navigator);
        nav.traverseAndSelect(content, selectedItem);
        window.requestAnimationFrame(() =>
        {
            const selectedElement = content.querySelector("[data-name='" + selectedItem + "']");
            if(selectedElement)
                selectedElement.scrollIntoView({block: "nearest", inline: "nearest"});
        });
    },

    resizeToContent(navigator)
    {
        const content = nav.getContent(navigator);
        if(!navigator || !content)
            return;

        const rect = navigator.getBoundingClientRect();
        const style = window.getComputedStyle(navigator);
        const borderHeight = parseFloat(style.borderTopWidth) + parseFloat(style.borderBottomWidth);
        const paddingHeight = parseFloat(style.paddingTop) + parseFloat(style.paddingBottom);
        const contentStyle = window.getComputedStyle(content);
        const contentTop = parseFloat(contentStyle.marginTop);
        const maxHeight = Math.max(120, window.innerHeight - rect.top - 12);
        const measuredHeight = Math.ceil(content.scrollHeight + contentTop + paddingHeight + borderHeight);

        navigator.style.setProperty("--navigator-open-height", Math.min(measuredHeight, maxHeight) + "px");
    },

    populateNavigator(navigator)
    {
        const content = nav.getContent(navigator);
        if(!content || !network || !network.network)
            return;
        content.innerHTML = "<ul>" + nav.buildList(network.network, "") + "</ul>";
        nav.revealSelection(navigator);
        nav.resizeToContent(navigator);
    },

    populate()
    {
        document.querySelectorAll("nav.pane_navigator").forEach((navigator) => nav.populateNavigator(navigator));
    }
};
