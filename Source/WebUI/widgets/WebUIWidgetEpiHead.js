class WebUIWidgetEpiHead extends WebUIWidgetGraph {
  static template() {
    return [
      { name: "SOURCE", control: "header" },
      { name: "title", default: "Epi Head", type: "string", control: "textedit" },
      { name: "eye_direction_source", default: "", type: "source", control: "textedit" },
      { name: "pupil_size_source", default: "", type: "source", control: "textedit" },
      { name: "left_eye_color_source", default: "", type: "source", control: "textedit" },
      { name: "right_eye_color_source", default: "", type: "source", control: "textedit" },
      { name: "top_mouth_color_source", default: "", type: "source", control: "textedit" },
      { name: "lower_mouth_color_source", default: "", type: "source", control: "textedit" },
      { name: "head_position_source", default: "", type: "source", control: "textedit" },
      { name: "PARAMETERS", control: "header" },
      { name: "eye_color", default: "#ffdd88", type: "string", control: "textedit" },
      { name: "mouth_color", default: "#ffdd88", type: "string", control: "textedit" },
      { name: "gaze", default: 0, type: "float", control: "slider", min: -45, max: 45 },
      { name: "vergence", default: 0, type: "float", control: "slider", min: -20, max: 20 },
      { name: "pupil_size_mm", default: 11, type: "float", control: "slider", min: 6, max: 16 },
      { name: "epi_name", default: "EpiRed", type: "string", control: "textedit" },
      { name: "STYLE", control: "header" },
      { name: "color", default: "black", type: "string", control: "textedit" },
      { name: "fill", default: "white", type: "string", control: "textedit" },
    ];
  }

  init() {
    super.init();

    // this.onclick = function () {
    //   alert(this.data);
    // }; // last matrix
  }

  flatNumbers(value, fallback, count) {
    const flattened = this.flattenSource(value);
    const numbers = flattened.map(Number).filter(Number.isFinite);
    const defaults = Array.isArray(fallback) ? fallback : [fallback];
    const result = [];
    for (let i = 0; i < count; i++)
      result.push(numbers[i] ?? numbers[0] ?? defaults[i] ?? defaults[0] ?? 0);
    return result;
  }

  colorByte(value) {
    const number = Number(value);
    return Number.isFinite(number) ? Math.round(Math.max(0, Math.min(1, number)) * 255) : 0;
  }

  draw() {
    let w = this.width;
    let h = this.height;
    let s = Math.min(this.width, this.height) / 180;
    let mw = Math.floor(0.5 * (w - s * 160)) + 0.5;
    let mh = Math.floor(0.5 * (h - s * 165)) + 0.5;

    this.resetCanvasTransform(0, 0);
    this.canvas.clearRect(-1, -1, this.width + 1, this.height + 1);
    this.setCanvasTransform(s, 0, 0, s, mw, mh);
    this.canvas.lineWidth = 1;

    // Move head
    this.canvas.translate(
      27.5 * Math.sin((this.head_position_source[0] * Math.PI) / 180),
      27.5 * Math.sin((this.head_position_source[1] * Math.PI) / 180)
    );

    // Figure out color of Epi.
    const epiName = String(this.parameters.epi_name ?? "EpiRed");
    let epiColor = epiName.startsWith("Epi") ? epiName.substring(3) : epiName;
    if (!epiColor)
      epiColor = "red";

    // left ear
    this.canvas.fillStyle = epiColor;
    this.canvas.beginPath();
    this.canvas.moveTo(3, 60);  
    this.canvas.lineTo(13, 60);   
    this.canvas.lineTo(13, 60+68);   
    this.canvas.lineTo(3, 60+68);  
    this.canvas.lineTo(0, 60+68-5);  
    this.canvas.lineTo(0, 60+5);  
    this.canvas.closePath();
    this.canvas.fill();
    this.canvas.stroke();
    this.canvas.beginPath();
    this.canvas.moveTo(3, 60);  
    this.canvas.lineTo(3, 60+68);  
    this.canvas.closePath(); 
    this.canvas.fill();
    this.canvas.stroke();

    // right ear
    this.canvas.fillStyle = epiColor;
    this.canvas.beginPath();
    this.canvas.moveTo(13+131, 60);  
    this.canvas.lineTo(13+131+10, 60);   
    this.canvas.lineTo(13+131+10+2.5, 60+5);   
    this.canvas.lineTo(13+131+10+2.5, 60+68-5);   
    this.canvas.lineTo(13+131+10, 60+68);   
    this.canvas.lineTo(13+131, 60+68);   
    this.canvas.closePath();
    this.canvas.fill();
    this.canvas.stroke();
    this.canvas.beginPath();
    this.canvas.moveTo(13+131+10, 60);  
    this.canvas.lineTo(13+131+10, 60+68);  
    this.canvas.closePath(); 
    this.canvas.stroke();

    // head
    this.setColor(0);
    this.canvas.beginPath();
    this.canvas.moveTo(13+40, 0);                       // 53,0
    this.canvas.lineTo(13+40+51, 0);                    // 104,0
    this.canvas.quadraticCurveTo(144, 0, 144, 40);      // 144,40
    this.canvas.lineTo(144, 40+81);                     // 144,91
    this.canvas.quadraticCurveTo(144, 163, 144-40, 163);   // 104,163
    this.canvas.lineTo(13+40, 163);                     // 53,163
    this.canvas.quadraticCurveTo(13, 163, 13, 40+81);     // 13,144
    this.canvas.lineTo(13, 40);                         // 13,40
    this.canvas.quadraticCurveTo(13, 0, 13+40, 0);      // 53,0
    this.canvas.closePath();
    this.canvas.fill();
    this.canvas.stroke();

    this.drawMouth = function (isTopLeds) {
      const sourceIndex = isTopLeds ? 0 : 1;
      const yOffset = isTopLeds ? 133 : 145;

    // first and last led is not visible in Epi
    for (let i = 0; i < 8; i++) {
      if (i === 0 || i === 7) continue;
      this.canvas.beginPath();
      this.canvas.arc(77 - 3.25 * 6.3 + i * 6.3, yOffset, 2, 0, 2 * Math.PI); // 3.25?
      this.canvas.fillStyle = this.mouth_colors[sourceIndex][i];
      this.canvas.fill();
      this.canvas.stroke();
    }
    };

    // Draw top mouth
    this.drawMouth(true);

    // Draw low mouth
    this.drawMouth(false);

    // Eyes
    // ====

    this.drawEye = function (isRightEye) {
      const sourceIndex = isRightEye ? 1 : 0;
      const eyeOffsetX = isRightEye ? 154-52.5+3-20 : 0+20;
      const eyeOffsetY = 71.5;

      // Eye outline
      this.canvas.save();
      this.canvas.translate(eyeOffsetX, eyeOffsetY);
      this.setColor(0);
      this.canvas.fillStyle = "white";
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
      this.canvas.clip(); // Woll make it hard to click

      // gaze
      this.canvas.translate(27.5, 20); // Translate to center of the eye

      this.canvas.translate(27.5 * Math.sin((this.gaze[sourceIndex] * Math.PI) / 180),0 ); // Change angle
      const scaleX = 1 - Math.abs(this.gaze[sourceIndex] * 0.006); // add angle effect on pupil. Assuming angle input max 45 degrees.
      this.canvas.transform(scaleX, 0, 0, 1, 0, 0);

      // Draw the NeoPixel ring
      const neoPixelRing = [];
      const centerX = 0;
      const centerY = 0;

    // Add Led diffuser
    this.canvas.fillStyle = "rgb(255, 255, 255)";
    this.canvas.beginPath();
    this.canvas.arc(centerX, centerY, 18, 0, 2 * Math.PI);
    this.canvas.stroke();
    this.canvas.fill();
      
      const radius = 8 + 5; // Radius of the iris plus some offset for the NeoPixel ring
      const ledRadius = 3.5;
      const numLEDs = 12;

      for (let i = 0; i < numLEDs; i++) {
        const angle = (i / numLEDs) * 2 * Math.PI;
        const x = centerX + radius * Math.cos(angle);
        const y = centerY + radius * Math.sin(angle);
        neoPixelRing.push({ x: x, y: y });
      }

      neoPixelRing.forEach((led, index) => {
        this.canvas.beginPath();
        this.canvas.arc(led.x, led.y, ledRadius, 0, 2 * Math.PI);
        this.canvas.fillStyle = this.eye_colors[sourceIndex][index];
        this.canvas.fill();
      });


          // Pupil
        this.canvas.fillStyle = "black";
        this.canvas.beginPath();
        this.canvas.arc(0, 0, this.pupil[sourceIndex] * 0.6, 0, 2 * Math.PI);
        this.canvas.fill();

      this.canvas.restore();



      // Draw eye outline
      this.canvas.save();
      this.canvas.translate(eyeOffsetX, eyeOffsetY);
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
      this.canvas.stroke();
      this.canvas.restore();
    };

    // Draw left eye
    this.drawEye(false);

    // Draw right eye
    this.drawEye(true);
  }

  update() {

    const rgbToHex = (r, g, b) => {
      return `#${[r, g, b].map((value) => this.colorByte(value).toString(16).padStart(2, "0")).join("")}`;
    };

    // Get values from source input or fill it with default values
    // Eyes
    let defaulteye_color = String(this.parameters.eye_color ?? "").split(',').map((c) => c.trim()).filter((c) => c !== ""); // Is this suppose to get the default value from template?
    if (defaulteye_color.length === 0 || (defaulteye_color.length === 1 && defaulteye_color[0] === '')) {
        defaulteye_color = ['yellow'];
    }
    if (defaulteye_color.length == 12)
         this.eye_colors = [defaulteye_color, defaulteye_color];
    else
        this.eye_colors = [Array(12).fill(defaulteye_color[0]), Array(12).fill(defaulteye_color[0])];
    
    let l_eye = this.getSource("left_eye_color_source", defaulteye_color);
    let r_eye = this.getSource("right_eye_color_source", defaulteye_color);
    
    // Convert the input to a hex representation of color 
    if (Array.isArray(l_eye) && l_eye.length === 3 && l_eye.every((channel) => Array.isArray(channel) && channel.length === 12))
      this.eye_colors[0] = l_eye[0].map((_, i) => rgbToHex(l_eye[0][i], l_eye[1][i], l_eye[2][i]));
    if (Array.isArray(r_eye) && r_eye.length === 3 && r_eye.every((channel) => Array.isArray(channel) && channel.length === 12))
      this.eye_colors[1] = r_eye[0].map((_, i) => rgbToHex(r_eye[0][i], r_eye[1][i], r_eye[2][i]));
    
    // Mouth
    let defaultmouth_color = String(this.parameters.mouth_color ?? "").split(',').map((c) => c.trim()).filter((c) => c !== "");
    if (defaultmouth_color.length === 0 || (defaultmouth_color.length === 1 && defaultmouth_color[0] === '')) {
        defaultmouth_color = ['yellow'];
    }
    if (defaultmouth_color.length == 8)
         this.mouth_colors = [defaultmouth_color, defaultmouth_color];
    else
        this.mouth_colors = [Array(8).fill(defaultmouth_color[0]), Array(8).fill(defaultmouth_color[0])];
    let t_mouth = this.getSource("top_mouth_color_source", defaultmouth_color);
    let l_mouth = this.getSource("lower_mouth_color_source", defaultmouth_color);
    // Convert the input to a hex representation of color
    if (Array.isArray(t_mouth) && t_mouth.length === 3 && t_mouth.every((channel) => Array.isArray(channel) && channel.length === 8))
      this.mouth_colors[0] = t_mouth[0].map((_, i) => rgbToHex(t_mouth[0][i], t_mouth[1][i], t_mouth[2][i]));
    if (Array.isArray(l_mouth) && l_mouth.length === 3 && l_mouth.every((channel) => Array.isArray(channel) && channel.length === 8))
      this.mouth_colors[1] = l_mouth[0].map((_, i) => rgbToHex(l_mouth[0][i], l_mouth[1][i], l_mouth[2][i]));

    // gaze. Only x wise and setting gaze in webUI will controll both eyes.
    const gazeValue = parseFloat(this.parameters.gaze);
    const vergenceValue = parseFloat(this.parameters.vergence);
    let defaultGaze = [
      (Number.isFinite(gazeValue) ? gazeValue : 0) - (Number.isFinite(vergenceValue) ? vergenceValue : 0),
      (Number.isFinite(gazeValue) ? gazeValue : 0) + (Number.isFinite(vergenceValue) ? vergenceValue : 0),
    ];
    this.gaze = this.flatNumbers(this.getSource("eye_direction_source", defaultGaze), defaultGaze, 2)
      .map((value) => Math.max(-90, Math.min(90, value)));
    
    // gaze. Only x wise and setting gaze in webUI will controll both eyes. // What happens if source input is only one element?
    const pupilValue = parseFloat(this.parameters.pupil_size_mm);
    let defaultPupil = [Number.isFinite(pupilValue) ? pupilValue : 11, Number.isFinite(pupilValue) ? pupilValue : 11];
    this.pupil = this.flatNumbers(this.getSource("pupil_size_source", defaultPupil), defaultPupil, 2)
      .map((value) => Math.max(0, Math.min(30, value)));

    // Head position. Fake tilt and pan of the robot. One value is treated as only tilt and two values tilt and pan.
    this.head_position_source = this.flatNumbers(this.getSource("head_position_source", [0, 0]), [0, 0], 2)
      .map((value) => Math.max(-90, Math.min(90, value)));
    this.draw();
  }
}

webui_widgets.add("webui-widget-epi-head", WebUIWidgetEpiHead);
