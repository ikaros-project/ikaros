/*
function getTextWidth(text, font) {
    // re-use canvas object for better performance
    var canvas = getTextWidth.canvas || (getTextWidth.canvas = document.createElement("canvas"));
    var context = canvas.getContext("2d");
    context.font = font;
    var metrics = context.measureText(text);
    return metrics.width;
}
*/

class WebUIWidgetSliderHorizontal extends WebUIWidgetControl
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'parameter', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'count', 'default':1, 'type':'int', 'control': 'textedit'},
            {'name':'title', 'default':"Sliders", 'type':'string', 'control': 'textedit'},
            {'name':'min', 'default':0, 'type':'float', 'control': 'textedit'},
            {'name':'max', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name':'step', 'default':0.01, 'type':'float', 'control': 'textedit'},
            {'name': "STYLE", 'control':'header'},
            {'name':'labels', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'show_values', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_title', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]};

    static html()
    {
         return `<div class="hranger">
                    <div><span class="slider_label"></span><input type="range"><span class="slider_value">0</span></div>
                    <div><span class="slider_label"></span><input type="range"><span class="slider_value">0</span></div>
                    <div><span class="slider_label"></span><input type="range"><span class="slider_value">0</span></div>
                    <div><span class="slider_label"></span><input type="range"><span class="slider_value">0</span></div>
                    <div><span class="slider_label"></span><input type="range"><span class="slider_value">0</span></div>
                    <div><span class="slider_label"></span><input type="range"><span class="slider_value">0</span></div>
                    <div><span class="slider_label"></span><input type="range"><span class="slider_value">0</span></div>
                </div>`;
    }
    update()
    {
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
        
        if(this.parameters.labels == "")
            for(let label of this.querySelectorAll(".slider_label"))
                label.style.display = 'none';
        else
        {
            let l = this.parameters.labels.split(',');
            let i=0;
            for(let label of this.querySelectorAll(".slider_label"))
            {
                label.style.display = 'block';
                label.innerText = l[i++].trim();
            }
        }
        
        for(let value of this.querySelectorAll(".slider_value"))
        {
            value.style.display = this.parameters.show_values ? 'block' : 'none'
            value.innerText = value.parentNode.children[1].value;
        }
    }
};



webui_widgets.add('webui-widget-slider-horizontal', WebUIWidgetSliderHorizontal);

