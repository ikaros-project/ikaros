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
        if(typeof view_menu !== "undefined")
            view_menu.hide();
        if(app_menu.dropdown)
        {
            app_menu.positionDropdown();
            app_menu.dropdown.classList.add("visible");
        }
    },

    hide()
    {
        if(app_menu.dropdown)
            app_menu.dropdown.classList.remove("visible");
    },

    positionDropdown()
    {
        if(app_menu.button && app_menu.dropdown)
            app_menu.dropdown.style.left = app_menu.button.offsetLeft + "px";
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
        else if(action === "savestate")
            controller.saveState();
        else if(action === "loadstate")
            controller.loadState();
        else if(action === "quit")
            controller.quit();
    }
};

const view_menu =
{
    init()
    {
        view_menu.button = document.getElementById("view_menu");
        view_menu.dropdown = document.getElementById("view_menu_dropdown");

        if(view_menu.button)
            view_menu.button.addEventListener("click", view_menu.toggle, false);

        document.addEventListener("mousedown", function(evt)
        {
            if(!view_menu.dropdown || !view_menu.button)
                return;
            if(view_menu.button.contains(evt.target) || view_menu.dropdown.contains(evt.target))
                return;
            view_menu.hide();
        }, true);

        document.addEventListener("keydown", function(evt)
        {
            if(evt.key === "Escape")
                view_menu.hide();
        }, true);
    },

    toggle(evt)
    {
        if(evt)
        {
            evt.preventDefault();
            evt.stopPropagation();
        }

        if(!view_menu.dropdown)
            return;

        if(view_menu.dropdown.classList.contains("visible"))
            view_menu.hide();
        else
            view_menu.show();
    },

    show()
    {
        app_menu.hide();
        if(view_menu.dropdown)
        {
            view_menu.positionDropdown();
            view_menu.dropdown.classList.add("visible");
        }
    },

    hide()
    {
        if(view_menu.dropdown)
            view_menu.dropdown.classList.remove("visible");
    },

    positionDropdown()
    {
        if(view_menu.button && view_menu.dropdown)
            view_menu.dropdown.style.left = view_menu.button.offsetLeft + "px";
    },

    toggleFullscreen()
    {
        const fullscreenElement = document.fullscreenElement || document.webkitFullscreenElement;
        if(fullscreenElement)
        {
            if(document.exitFullscreen)
                document.exitFullscreen();
            else if(document.webkitExitFullscreen)
                document.webkitExitFullscreen();
            return;
        }

        const target = document.documentElement;
        if(target.requestFullscreen)
            target.requestFullscreen();
        else if(target.webkitRequestFullscreen)
            target.webkitRequestFullscreen();
    },

    choose(action)
    {
        view_menu.hide();

        if(action === "hide_toolbar" && main && typeof main.toggleTopChrome === "function")
            main.toggleTopChrome();
        else if(action === "fullscreen")
            view_menu.toggleFullscreen();
        else if(action === "show_profiling" && typeof inspector !== "undefined" && typeof inspector.openProfilingWindow === "function")
            inspector.openProfilingWindow();
        else if(action === "show_module_start" && typeof inspector !== "undefined" && typeof inspector.openStartupStepsWindow === "function")
            inspector.openStartupStepsWindow();
        else if(action === "reset_panes" && main && typeof main.resetSavedPaneLayout === "function")
            main.resetSavedPaneLayout();
    }
};
