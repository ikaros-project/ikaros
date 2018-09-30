class WebUIWidgetEpiHead extends WebUIWidgetGraph
{
    static template()
    {
        return [
            {'name': "PARAMETERS", 'control':'header'},
            {'name':'module', 'default':"", 'type':'source', 'control': 'textedit'},
            {'name':'title', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'size', 'default':1, 'type':'float', 'control': 'textedit'},
            {'name': "STYLE", 'control':'header'},
            {'name':'color', 'default':"black", 'type':'string', 'control': 'textedit'},
            {'name':'fill', 'default':"white", 'type':'string', 'control': 'textedit'},
            {'name':'earColor', 'default':"#6666ff", 'type':'string', 'control': 'textedit'},
            {'name':'eyeColor', 'default':"#8888ff", 'type':'string', 'control': 'textedit'},
            {'name':'show_title', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'show_frame', 'default':true, 'type':'bool', 'control': 'checkbox'},
            {'name':'style', 'default':"", 'type':'string', 'control': 'textedit'},
            {'name':'frame-style', 'default':"", 'type':'string', 'control': 'textedit'}
        ]
    };


    init()
    {
        super.init();

        this.onclick = function () { alert(this.data) }; // last matrix
    }


    requestData(data_set)
    {
//        data_set.add(this.parameters['module']+".KEYPOINTS");
//        data_set.add(this.parameters['module']+".TIMESTAMPS");
    }


    drawEye(x, y) // subtract 22.5, 72.5 later;   37.5, 60 - 72.5
    {
        // Eye outline

        this.canvas.save();
        this.canvas.translate(x, y);
        this.setColor(0);
        this.canvas.beginPath();
        this.canvas.moveTo(37.5, 0);
        this.canvas.quadraticCurveTo(52.5, 0, 52.5, 15);
        this.canvas.lineTo(52.5, 25);
        this.canvas.quadraticCurveTo(52.5, 40, 37.5, 40);
        this.canvas.lineTo(16, 40);
        this.canvas.quadraticCurveTo(0, 40, 0, 25);
        this.canvas.lineTo(0, 15);
        this.canvas.quadraticCurveTo(0, 0, 15, 0);
        this.canvas.closePath();
        this.canvas.fill();
        this.canvas.stroke();
        this.canvas.clip();
        
        this.canvas.translate(0, 0); // gaze
        
        // Iris
        
        this.canvas.fillStyle = this.parameters.eyeColor; // when LED input not connected
        this.canvas.strokeStyle = "#00000000";
        this.canvas.beginPath();
        this.canvas.arc(27.5, 20, 17.5, 0, 2*Math.PI);
        this.canvas.fill();

        // Pupil

        this.canvas.fillStyle = "black";
        this.canvas.beginPath();
        this.canvas.arc(27.5, 20, 7.5, 0, 2*Math.PI);
        this.canvas.fill();

        this.canvas.restore();
    }
    
    
    draw(count,channels)
    {
        let w = this.width;
        let h = this.height;
        
        this.canvas.setTransform(1, 0, 0, 1, 15-0.5, 15-0.5);
        this.canvas.clearRect(0, 0, this.width, this.height);
//        this.canvas.translate(this.format.marginLeft, this.format.marginTop);

        this.canvas.lineWidth = 1;
//        this.canvas.clip();

        // ears
        
        this.setColor(0);
        this.canvas.fillStyle = this.parameters.earColor;
 
        this.canvas.rect(5, 60, 10, 65);
        this.canvas.rect(145, 60, 10, 65);
        this.canvas.fill();
        this.canvas.stroke();
        
       // head

        this.setColor(0);
        this.canvas.beginPath();

        this.canvas.moveTo(50, 0);
        this.canvas.lineTo(110, 0);
        this.canvas.quadraticCurveTo(145, 0, 145, 50);
        this.canvas.lineTo(145, 126);
        this.canvas.quadraticCurveTo(145, 165, 110, 165);
        this.canvas.lineTo(50, 165);
        this.canvas.quadraticCurveTo(15, 165, 15, 125);
        this.canvas.lineTo(15, 40);
        this.canvas.quadraticCurveTo(15, 0, 50, 0);
        this.canvas.closePath();
        
        this.canvas.moveTo(5, 62.5);
        this.canvas.lineTo(0, 67.5);
        this.canvas.lineTo(0, 117.5);
        this.canvas.lineTo(5, 122.5);
        this.canvas.closePath();

        this.canvas.moveTo(5, 62.5);
        this.canvas.lineTo(0, 67.5);
        this.canvas.lineTo(0, 117.5);
        this.canvas.lineTo(5, 122.5);
        this.canvas.closePath();
        
        this.canvas.moveTo(155, 62.5);
        this.canvas.lineTo(160, 67.5);
        this.canvas.lineTo(160, 117.5);
        this.canvas.lineTo(155, 122.5);
        this.canvas.closePath();
        
        this.canvas.fill();
        this.canvas.stroke();

        // Mouth
        
        for(let c=0; c<2; c++)
            for(let i=0; i<6; i++)
            {
                this.canvas.beginPath();
                this.canvas.arc(64+i*6.25, 135+c*10, 2, 0, 2*Math.PI);
                this.canvas.stroke();
            }
        
        this.drawEye(22.5, 72.5); // left eye
        this.drawEye(85, 72.5); // right eye
    }


    update(d)
    {
//        if(!d)
//            return;
        
        try {
        //    let m = this.parameters['module'];

            this.draw();
        }
        catch(err)
        {
            this.draw();
        }
    }
};


webui_widgets.add('webui-widget-epi-head', WebUIWidgetEpiHead);
