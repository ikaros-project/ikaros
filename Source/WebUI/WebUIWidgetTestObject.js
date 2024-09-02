class WebUIWidgetTestObject extends WebUIWidgetCanvas
{

    static template()
    {
        return [
            {'name':'source', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'alpha', 'default':0.5, 'type':'float', 'control': 'textedit'},
            {'name':'iota', 'default':2, 'type':'int', 'control': 'textedit'},
            {'name':'red', 'default':255, 'type':'int', 'min':0, 'max':255, 'control': 'slider'},
            {'name':'green', 'default':127, 'type':'int', 'min':0, 'max':255, 'control': 'slider'},
            {'name':'blue', 'default':32, 'type':'int', 'min':0, 'max':255, 'control': 'slider'},
            {'name':'enjoy', 'default':false, 'type':'bool', 'control': 'checkbox'},
            {'name':'count', 'default':0, 'type':'int', 'min':0, 'max':12, 'control': 'number'},
            {'name':'blend', 'default':'absolute', 'type':'string', 'control': 'menu', 'options': "absolute,monochrome,mix"},
            {'name':'direction', 'default':'all', 'type':'string-öh', 'min':0, 'max':2, 'control': 'menu', 'options': "all,horizontal,vertical"}
        ]};


    init()
    {
        this.data = [10, 83, 32, 56];   // Should connect to main data structure
        
        this.onclick = function () { alert(this.data) };
        
        var self = this;
        this.timer = setInterval(function () { self.draw(); console.log("draw callback"); }, 100);
    }


    update()
    {
        let width = parseInt(getComputedStyle(this.canvasElement).width);
        let height = parseInt(getComputedStyle(this.canvasElement).height);
        if(width != this.canvasElement.width || height != this.canvasElement.height)
        {
            this.canvasElement.width = parseInt(width);
            this.canvasElement.height = parseInt(height);
        }
        
        this.width = parseInt(width);
        this.height = parseInt(height);

        this.draw();
    }


    draw()
    {
        let r = this.parameters.red;
        let g = this.parameters.green;
        let b = this.parameters.blue;

        let c = "white";
        switch(this.parameters.blend)
        {
            case "absolute":
                c = "rgb("+r+","+g+","+b+")";
                break;
            case "monochrome":
                let rnd = Math.random();
                c = "rgb("+Math.round(r*rnd)+","+Math.round(g*rnd)+","+Math.round(b*rnd)+")";
                break;
            case "mix":
                c = "rgb("+Math.round(r*Math.random())+","+Math.round(g*Math.random())+","+Math.round(b*Math.random())+")";
                break;
        }

        let lineWidth = getComputedStyle(this.canvasElement).getPropertyValue('--line-width');

        this.canvas.beginPath();
        this.canvas.lineWidth = lineWidth;
        this.canvas.strokeStyle = c;

        switch(this.parameters.direction)
        {
            case "all":
                this.canvas.moveTo(this.width*Math.random(), this.height*Math.random());
                this.canvas.lineTo(this.width*Math.random(), this.height*Math.random());
                break;
            case "horizontal":
                this.canvas.moveTo(0, this.height*Math.random());
                this.canvas.lineTo(this.width, this.height*Math.random());
                break;
            case "vertical":
                this.canvas.moveTo(this.width*Math.random(), 0);
                this.canvas.lineTo(this.width*Math.random(), this.height);
                break;
        }
        this.canvas.stroke();
    }
};



webui_widgets.add('webui-widget-test-object', WebUIWidgetTestObject);
