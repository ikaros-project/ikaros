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

class WebUIWidgetSliderHorizontal extends WebUIWidgetControl {
    static template() {
        return [
            { 'name': "SLIDER HORIZONTAL", 'control': 'header' },
            { 'name': 'title', 'default': "Sliders", 'type': 'string', 'control': 'textedit' },

            { 'name': "CONTROL", 'control': 'header' },
            { 'name': 'parameter', 'default': "", 'type': 'source', 'control': 'textedit' },
            { 'name': 'select', 'default': 0, 'type': 'int', 'control': 'textedit' },
            { 'name': 'count', 'default': 1, 'type': 'int', 'control': 'textedit' },

            { 'name': "STYLE", 'control': 'header' },
            { 'name': 'labels', 'default': "", 'type': 'string', 'control': 'textedit' },
            { 'name': 'min', 'default': 0, 'type': 'float', 'control': 'textedit' },
            { 'name': 'max', 'default': 1, 'type': 'float', 'control': 'textedit' },
            { 'name': 'step', 'default': 0.01, 'type': 'float', 'control': 'textedit' },
            { 'name': 'show_values', 'default': false, 'type': 'bool', 'control': 'checkbox' },

            { 'name': "FRAME", 'control': 'header' },
            { 'name': 'show_title', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'show_frame', 'default': false, 'type': 'bool', 'control': 'checkbox' },
            { 'name': 'style', 'default': "", 'type': 'string', 'control': 'textedit' },
            { 'name': 'frame-style', 'default': "", 'type': 'string', 'control': 'textedit' }
        ]
    };

    static html() {
        return `<div class="hranger"></div>`;
    }

    slider_moved(value, index = 0) {
        this.is_active = true;
        this.send_control_change(this.parameters.parameter, value, this.parameters.select + index);

        // moves all sliders at ones. 
        if (this.sync) {
            var newValue = value;
            for (let i = 0; i < this.parameters.count; i++) {
                this.send_control_change(this.parameters.parameter, newValue, this.parameters.select + i);
                this.querySelectorAll("input")[i].value = newValue;
            }
        }
    }

    updateAll() {
        super.updateAll();

        // add or remove sliders
        while (this.firstChild.childElementCount > this.parameters.count)
            this.firstChild.removeChild(this.firstChild.children[this.firstChild.childElementCount - 1]);

        while (this.firstChild.childElementCount < this.parameters.count) {
            let d = document.createElement("div");
            d.innerHTML = '<span class="slider_label"></span><input type="range"><span class="slider_value">0</span>';
            this.firstChild.insertBefore(d, null);
        }

        // This should only be done on change

        for (let slider of this.querySelectorAll("input")) {
            slider.min = this.parameters.min;
            slider.max = this.parameters.max;
            slider.step = this.parameters.step;
        }

        let mode = this.parameters.labels.trim() == "" ? 'none' : 'block';
        for (let label of this.querySelectorAll(".slider_label"))
            label.style.display = mode;

        for (let label of this.querySelectorAll(".slider_label"))
            try {
                label.innerText = l[i++].trim();
            }
            catch (err) {
                label.innerText = "";
            }

        if (this.parameters.labels.trim() == "")
            for (let label of this.querySelectorAll(".slider_label")) {
                label.style.display = 'none';
                label.innerText = "";
            }
        else {
            let l = this.parameters.labels.split(',');
            let i = 0;
            for (let label of this.querySelectorAll(".slider_label")) {
                label.style.display = 'block';
                try {
                    label.innerText = l[i++].trim();
                }
                catch (err) {
                }
            }
        }

        for (let value of this.querySelectorAll(".slider_value")) {
            value.style.display = this.parameters.show_values ? 'block' : 'none';
        }

        // set-up event handlers
        document.addEventListener('keydown', (e) => {
            if (e.shiftKey) {
                this.sync = true;
            }
        });
        document.addEventListener('keyup', (e) => {
            if (!e.shiftKey) {
                this.sync = false;
            }
        });

        let i = 0;
        for (let slider of this.querySelectorAll("input")) {
            slider.index = i++;
            slider.oninput = function () {
                this.parentElement.parentElement.parentElement.slider_moved(this.value, this.index);
            };
            slider.onmousedown = function (e) { this.parentElement.parentElement.parentElement.is_active = false; e.stopPropagation(); console.log("slider down");};  
            slider.onmouseup = function (e) { this.parentElement.parentElement.parentElement.is_active = false; e.stopPropagation(); console.log("slider up");};
            slider.onclick = function (e) { 
                    this.is_active = false; 
                    e.stopPropagation(); 
                    console.log("slider click");}
                ;
        }

    }

    update() {
        if (this.parameters.show_values)
            for (let value of this.querySelectorAll(".slider_value"))
                value.innerText = value.parentNode.children[1].value;

        if (this.is_active) // Do not redraw from data while being tracked
            return;
        try {
            let d = this.getSource("parameter");

            // Hack if we have an array. We should have a better way to handle this.
            if (Array.isArray(d) && !Array.isArray(d[0])) {
                d = [d];
            }

            if (d) {
                let size_y = d.length;
                let size_x = d[0].length;

                let i = this.parameters.select;
                for (let slider of this.querySelectorAll("input"))
                    slider.value = d[0][i++];
            }
        }
        catch (err) {
        }
    }
};


webui_widgets.add('webui-widget-slider-horizontal', WebUIWidgetSliderHorizontal);

