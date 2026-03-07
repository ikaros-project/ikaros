class WebUIWidgetSliderVertical extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "SLIDER VERTICAL", 'control':'header'},
            {'name':'title', 'default':"Sliders", 'type':'string', 'control': 'textedit'},

            {'name': "CONTROL", 'control':'header'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'select', 'default':0, 'type':'int', 'control': 'textedit'},
            {'name':'count', 'default':1, 'type':'int', 'control': 'textedit'},
            
            {'name': "STYLE", 'control':'header'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'step', 'default':0.01, 'type':'float', 'control': 'textedit'},
            {'name':'show_values', 'default':false, 'type':'bool', 'control': 'checkbox'},

            {'name': "FRAME", 'control':'header'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
         return `<div class="vranger"></div>`;
    }

    slider_moved(value, index=0)
    {
        this.send_control_change(this.parameters.parameter, value, this.parameters.select + index);
    }

    updateAll()
    {
        super.updateAll();

        // add or remove sliders
        
        while(this.firstChild.childElementCount > this.parameters.count)
            this.firstChild.removeChild(this.firstChild.children[this.firstChild.childElementCount-1]);

        while(this.firstChild.childElementCount < this.parameters.count)
        {
            let d = document.createElement("div");
            d.innerHTML = '<span class="slider_label"></span><input type="range"><span class="slider_value">0</span>';
            this.firstChild.insertBefore(d, null);
        }
        
        // This should only be done on change
        
        for(let slider of this.querySelectorAll("input"))
        {
            slider.min = this.parameters.min;
            slider.max = this.parameters.max;
            slider.step = this.parameters.step;
        }

        const rawLabels = String(this.parameters.labels ?? "").trim();
        const labels = rawLabels === "" ? [] : rawLabels.split(",").map((item) => item.trim());
        const showLabels = labels.length > 0;
        let i = 0;
        for(let label of this.querySelectorAll(".slider_label"))
        {
            label.style.display = showLabels ? "block" : "none";
            label.innerText = labels[i++] ?? "";
        }

        for(let value of this.querySelectorAll(".slider_value"))
        {
            value.style.display = this.parameters.show_values ? 'block' : 'none';
        }
        
        // set-up event handlers

        let i = 0;
        for(let slider of this.querySelectorAll("input"))
        {
            slider.index = i++;
            slider.oninput = function (){
                this.parentElement.parentElement.parentElement.slider_moved(this.value, this.index);
            }
        }
    }

    update()
    {
         try {
            let d = this.getSource("parameter");
            if(d)
            {
                if (!Array.isArray(d))
                    return;
                if (!Array.isArray(d[0]))
                    d = [d];
                if (!Array.isArray(d[0]))
                    return;
                
                let i = this.parameters.select;
                for(let slider of this.querySelectorAll("input"))
                {
                    if (typeof d[0][i] !== "undefined")
                        slider.value = d[0][i];
                    i++;
                }
            }

            if(this.parameters.show_values)
                for(let value of this.querySelectorAll(".slider_value"))
                    value.innerText = value.parentNode.children[1].value;
        }
        catch(err)
        {
        
        }
    }
};



webui_widgets.add('webui-widget-slider-vertical', WebUIWidgetSliderVertical);
