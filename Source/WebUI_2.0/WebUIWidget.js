class WebUIWidget extends HTMLElement
{
    constructor()
    {
        super();
        let pt = this.constructor.template();
        this.parameters = {};
        for(let i in pt)
            this.parameters[pt[i].name] = pt[i]['default'];
        this.parameter_template = pt;
    }

    static html()
    {
        return `
            <style>
                div { background-color: rgba(0,0,0,0); color: red; }
            </style>
            <div>Widget</div>
        `;
    }

    static template()
    {
        return [
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    connectedCallback()
    {
        this.innerHTML = this.constructor.html();

        this.onmousedown = function () { console.log("WebUIWidgetCanvas: mouse down"); }
        this.onmouseup = function () { console.log("WebUIWidgetCanvas: mouse up"); }
        this.onclick = function () { console.log("WebUIWidgetCanvas: click"); }
        this.onmousemove = function () { console.log("WebUIWidgetCanvas: mousemove"); }
        this.onmouseover = function () { console.log("WebUIWidgetCanvas: mouseover"); }
        this.onmouseout = function () { console.log("WebUIWidgetCanvas: mouseout"); }

//        this.parentElement.setAttribute("data-name", "object #"+Math.round(1000*Math.random()));
//        this.redraw();
    }

    static get observedAttributes()
    {
        return [];
    }

    attributeChangedCallback(name, oldValue, newValue)
    {
    }
};

customElements.define('webui-widget', WebUIWidget);

