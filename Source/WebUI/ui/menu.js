const app_menu =
{
    init()
    {
        app_menu.button = document.getElementById("menu");
        app_menu.dropdown = document.getElementById("menu_dropdown");

        if(app_menu.button)
            app_menu.button.addEventListener("click", app_menu.toggle, false);

        document.addEventListener("mousedown", function(evt)
        {
            if(!app_menu.dropdown || !app_menu.button)
                return;
            if(app_menu.button.contains(evt.target) || app_menu.dropdown.contains(evt.target))
                return;
            app_menu.hide();
        }, true);

        document.addEventListener("keydown", function(evt)
        {
            if(evt.key === "Escape")
                app_menu.hide();
        }, true);
    },

    toggle(evt)
    {
        if(evt)
        {
            evt.preventDefault();
            evt.stopPropagation();
        }

        if(!app_menu.dropdown)
            return;

        if(app_menu.dropdown.classList.contains("visible"))
            app_menu.hide();
        else
            app_menu.show();
    },

    show()
    {
        if(app_menu.dropdown)
            app_menu.dropdown.classList.add("visible");
    },

    hide()
    {
        if(app_menu.dropdown)
            app_menu.dropdown.classList.remove("visible");
    },

    choose(action)
    {
        app_menu.hide();

        if(action === "new")
            controller.new();
        else if(action === "open")
            controller.open();
        else if(action === "save")
            controller.save();
        else if(action === "saveas")
            controller.saveas();
        else if(action === "quit")
            controller.quit();
    }
};
