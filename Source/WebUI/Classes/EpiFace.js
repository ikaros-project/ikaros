function EpiFace(p)
{
    var that = this;
    

/*
    function none()
    {
    }
    
	function down(e)
	{
        e.preventDefault();
        that.clicked = true;
        that.button.setAttribute("opacity", 0);
        that.label.setAttribute('y', that.height/2+6);
        if(that.module && that.parameter)
            get("/control/"+that.module+"/"+that.parameter+"/"+that.xindex+"/"+that.yindex+"/"+that.max, none);
        document.onmouseup = up;
	}
    
	function up()
	{
        that.button.setAttribute("opacity", 1);
        that.label.setAttribute('y', that.height/2+4);
        document.onmouseup = null;
        if(that.module && that.parameter)
            get("/control/"+that.module+"/"+that.parameter+"/"+that.xindex+"/"+that.yindex+"/"+that.min, none);
        that.clicked = false;
	}
    

*/

    this.scale_svg_object = function(obj, factor)
    {
        b = obj.getBBox();
        cx = b.x + 0.5 * b.width;
        cy = b.y + 0.5 * b.height;
        dx = -cx * (factor-1);
        dy = -cy * (factor-1);
        t = "translate("+dx+","+dy+")"+" scale("+factor+")";
        obj.setAttribute("transform", t)
    }

    function touch_start(event){down(event);}
    function touch_end(event){up();}
    
    this.title = (p.title ? p.title : p.module+'.'+p.parameter);

    p.opaque = true;
    this.graph = new Graph(p, this.title);

    this.left_module = p.left_module;
	this.left_source = p.left_source;
    this.right_module = p.right_module;
	this.right_source = p.right_source;

    this.parameter = p.parameter;
    this.x = p.x;
    this.y = p.y;	
    this.width = p.width;
    this.height = p.height;
    this.min = (p.min ? p.min : 0);
    this.max = (p.max ? p.max : 1);
    this.select = makeSelectionArray(p.select);
    this.xindex = (p.xindex ? p.xindex : this.select[0][0]);
    this.yindex = (p.yindex ? p.yindex : this.select[0][1]);
    
    this.left_eye = null;
    this.right_eye = null;
    this.left_pupil = null;
    this.right_pupil = null;

    this.counter = 0;

/*
    this.face_pressed = document.createElementNS(svgns,"image");	
    this.face_pressed.setAttribute('x', 0);
    this.face_pressed.setAttribute('y', 0);
    this.face_pressed.setAttribute('width', this.width);
    this.face_pressed.setAttribute('height', this.height);
    this.face_pressed.setAttribute('preserveAspectRatio', 'none');
    this.face_pressed.setAttributeNS(xlinkns, 'href', "/Classes/button_pressed.svg");
    this.face_pressed.setAttribute('style', 'cursor: pointer');
    this.graph.group.appendChild(this.face_pressed);
*/

    var r = document.createElementNS(svgns,"foreignObject");
    r.setAttribute('x', 0);
    r.setAttribute('y', 0);
    r.setAttribute('width', this.width);
    r.setAttribute('height', this.height);
    
    var b = document.createElementNS(xmlns,"body");
    b.setAttribute('xmlns', 'http://www.w3.org/1999/xhtml');
    
    r.appendChild(b);
    this.graph.group.appendChild(r);

// <object id="svgObject" data="epi_face.svg" type="image/svg+xml" height="250" width="250">

    this.face = document.createElementNS(xmlns,"object");
    this.face.setAttribute('x', 0);
    this.face.setAttribute('y', 0);
    this.face.setAttribute('width', this.width);
    this.face.setAttribute('height', this.height);
    this.face.setAttribute('type', 'image/svg+xml');
    this.face.setAttribute('data', "/Classes/epi_face.svg");
    this.face.setAttribute('style', 'cursor: pointer');
    this.face.setAttribute('id', 'svgobj');
    this.face.onload = function () {
        that.left_eye = that.face.contentDocument.getElementById("LeftEye");
        that.right_eye = that.face.contentDocument.getElementById("RightEye");
        that.left_pupil =  that.face.contentDocument.getElementById("LeftPupil");
        that.right_pupil =  that.face.contentDocument.getElementById("RightPupil");
        
//        console.log(that.right_pupil.getBoundingClientRect());
        
        // Sccale pupil the hard way
/*
        scale_svg_object(that.left_pupil, 1.5);
        scale_svg_object(that.right_pupil, 1.5);
        
        that.left_eye.setAttribute("transform", "translate(10)");
        that.right_eye.setAttribute("transform", "translate(10)");
*/
    }
    b.appendChild(this.face);
    
    
//    this.face.onmousedown = down;
//    this.face.onmouseup = up;
    this.value = 0;
    this.clicked = false;

//    this.face.addEventListener('touchstart', function(event){event.preventDefault(); touch_start(event)}, false);
//    this.face.addEventListener('touchend', function(event) {event.preventDefault(); touch_end(event);}, false);

    usesData(this.left_module, this.left_source);
    usesData(this.right_module, this.right_pupil_source);
}


EpiFace.prototype.Update = function(data)
{
    var d = data[this.left_module];
    if(d)
    {
        d = d[this.left_source]
        if(d)
        {
            this.left_eye.setAttribute("transform", "translate("+d[0][0]+", "+d[0][1]+")");
            this.scale_svg_object(this.left_pupil, d[0][2]);
        }
    }
    
    var d = data[this.right_module];
    if(d)
    {
        d = d[this.right_source]
        if(d)
        {
            this.right_eye.setAttribute("transform", "translate("+d[0][0]+", "+d[0][1]+")");
            this.scale_svg_object(this.right_pupil, d[0][2]);
        }
    }
}


