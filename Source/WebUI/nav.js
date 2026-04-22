const nav =
{
    init()
    {
        nav.navigator = document.getElementById('navigator');
        if(nav.navigator)
            nav.navigator.addEventListener('click', nav.handleClick, false);
    },

    toggle()
    {
        const s = window.getComputedStyle(nav.navigator, null);
        if(s.display === 'none')
            nav.navigator.style.display = 'block';
        else
            nav.navigator.style.display = 'none';
    },

    toggleGroup(e)
    {
        if(e.target.classList.contains("group-open"))
            e.target.classList.replace("group-open", "group-closed");
        else if(e.target.classList.contains("group-closed"))
            e.target.classList.replace("group-closed", "group-open");

        e.stopPropagation();
    },

    openGroup(item)
    {
        let g = nav.navigator.querySelector("[data-name='" + item + "']");
        g = g.parentElement;
        while(g)
        {
            g.classList.remove("group-closed");
            g.classList.add("group-open");
            g = g.parentElement;
        }
    },

    selectItem(item)
    {
        nav.traverseAndSelect(nav.navigator, item);
        nav.traverseAndOpen(nav.navigator, item);
    },

    selectModule(evt)
    {
    },

    navClick(e)
    {
        const bg = e.target.parentElement.dataset.name;
        selector.selectItems([], bg);
        e.stopPropagation();
    },

    handleClick(e)
    {
        const label = e.target.closest("span");
        if(label && nav.navigator.contains(label))
        {
            nav.navClick(e);
            return;
        }

        const groupItem = e.target.closest("li[data-name]");
        if(groupItem && nav.navigator.contains(groupItem) && !groupItem.classList.contains("group-empty"))
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

    populate()
    {
        nav.navigator.innerHTML = "<ul>" + nav.buildList(network.network, "") + "</ul>";
    }
};
