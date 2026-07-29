class WebUIWidgetAngularHUD extends WebUIWidgetCanvas
{
    init()
    {
        super.init();
        this.style.pointerEvents = "none";
    }

    normalizeAngle(value, minimum=0, period=360)
    {
        const numeric = Number(value);
        if(!Number.isFinite(numeric) || !Number.isFinite(period) || period <= 0)
            return null;
        return ((numeric - minimum) % period + period) % period + minimum;
    }

    signedAngleDifference(target, current, period=360)
    {
        const targetValue = Number(target);
        const currentValue = Number(current);
        if(!Number.isFinite(targetValue) || !Number.isFinite(currentValue) || !Number.isFinite(period) || period <= 0)
            return null;
        return ((targetValue - currentValue + period / 2) % period + period) % period - period / 2;
    }

    sourceValue(name, index=0)
    {
        const source = this.getSource(name);
        const value = Number(this.sourceScalar(source, index));
        return Number.isFinite(value) ? value : null;
    }

    angleToDegrees(value)
    {
        if(value === null || value === undefined)
            return null;
        const numeric = Number(value);
        if(!Number.isFinite(numeric))
            return null;
        return this.parameters.angle_unit === "radians" ? numeric * 180 / Math.PI : numeric;
    }

    formatHUDValue(value, decimals=0, signed=false)
    {
        if(!Number.isFinite(value))
            return "—";
        const precision = Math.max(0, Math.min(20, Math.trunc(Number(decimals) || 0)));
        const text = value.toFixed(precision);
        return signed && value > 0 ? `+${text}` : text;
    }

    getHUDColor(name, fallback)
    {
        const value = String(this.parameters[name] ?? "").trim();
        return value || fallback;
    }

    drawOutlinedText(text, x, y, options={})
    {
        this.canvas.save();
        this.canvas.font = options.font || this.parameters.hud_font || "13px sans-serif";
        this.canvas.textAlign = options.align || "center";
        this.canvas.textBaseline = options.baseline || "middle";
        const outline = this.getHUDColor("outline_color", "rgba(0,0,0,0.85)");
        const width = Math.max(0, Number(this.parameters.outline_width) || 0);
        if(width > 0)
        {
            this.canvas.lineWidth = 2 * width;
            this.canvas.lineJoin = "round";
            this.canvas.strokeStyle = outline;
            this.canvas.strokeText(String(text), x, y);
        }
        this.canvas.fillStyle = options.color || "white";
        this.canvas.fillText(String(text), x, y);
        this.canvas.restore();
    }

    drawTriangle(x, tipY, direction, size, color)
    {
        const sign = direction === "down" ? -1 : 1;
        this.canvas.save();
        this.canvas.beginPath();
        this.canvas.moveTo(x, tipY);
        this.canvas.lineTo(x - size, tipY + sign * size * 1.5);
        this.canvas.lineTo(x + size, tipY + sign * size * 1.5);
        this.canvas.closePath();
        this.canvas.fillStyle = color;
        this.canvas.strokeStyle = this.getHUDColor("outline_color", "rgba(0,0,0,0.85)");
        this.canvas.lineWidth = Math.max(1, Number(this.parameters.outline_width) || 1);
        this.canvas.fill();
        this.canvas.stroke();
        this.canvas.restore();
    }

    isMajorTick(value, step)
    {
        const divisor = Math.max(Number(step) || 1, Number.EPSILON);
        return Math.abs(value / divisor - Math.round(value / divisor)) < 1e-6;
    }

    headingLabel(value, showCardinals=true)
    {
        const normalized = this.normalizeAngle(value, 0, 360);
        if(normalized === null)
            return "";
        const rounded = Math.round(normalized) % 360;
        if(showCardinals)
        {
            if(rounded === 0)
                return "N";
            if(rounded === 90)
                return "E";
            if(rounded === 180)
                return "S";
            if(rounded === 270)
                return "W";
        }
        return String(rounded).padStart(3, "0");
    }

    panLabel(value)
    {
        const rounded = Math.round(value);
        return rounded > 0 ? `+${rounded}` : String(rounded);
    }

    drawHorizontalTape(options)
    {
        const current = Number(options.current);
        if(!Number.isFinite(current))
            return;
        const left = Number(options.left);
        const right = Number(options.right);
        const centerX = (left + right) / 2;
        const width = right - left;
        const fieldOfView = Math.max(1, Number(options.fieldOfView) || 120);
        const minorStep = Math.max(0.1, Number(options.minorStep) || 5);
        const majorStep = Math.max(minorStep, Number(options.majorStep) || 30);
        const upper = options.side !== "lower";
        const baselineY = Number(options.baselineY);
        const separatorY = Number(options.separatorY);
        const tickDirection = upper ? 1 : -1;
        const color = options.color || "white";
        const halfView = fieldOfView / 2;
        const firstTick = Math.ceil((current - halfView) / minorStep) * minorStep;
        const lastTick = current + halfView;

        this.canvas.save();
        this.canvas.strokeStyle = color;
        this.canvas.fillStyle = color;
        this.canvas.lineWidth = Math.max(1, Number(this.parameters.line_width) || 1);
        this.canvas.beginPath();
        this.canvas.moveTo(left, baselineY);
        this.canvas.lineTo(right, baselineY);
        this.canvas.stroke();

        for(let value = firstTick; value <= lastTick + minorStep * 0.01; value += minorStep)
        {
            if(!options.wrap && (value < options.minimum - 1e-6 || value > options.maximum + 1e-6))
                continue;
            const x = centerX + (value - current) * width / fieldOfView;
            const major = this.isMajorTick(value, majorStep);
            const tickLength = major ? 10 : 5;
            this.canvas.beginPath();
            this.canvas.moveTo(x, baselineY);
            this.canvas.lineTo(x, baselineY + tickDirection * tickLength);
            this.canvas.stroke();
            if(major)
            {
                const label = options.labelFormatter(value);
                this.drawOutlinedText(label, x, baselineY - tickDirection * 8, {
                    color,
                    baseline:upper ? "bottom" : "top",
                    font:this.parameters.scale_font || "12px sans-serif"
                });
            }
        }

        this.canvas.strokeStyle = this.getHUDColor("separator_color", "rgba(255,255,255,0.45)");
        this.canvas.lineWidth = 1;
        this.canvas.beginPath();
        this.canvas.moveTo(left, separatorY);
        this.canvas.lineTo(right, separatorY);
        this.canvas.stroke();
        this.drawTriangle(centerX, separatorY + (upper ? -1 : 1), upper ? "down" : "up", 6, color);

        if(Number.isFinite(options.target))
        {
            const difference = options.wrap ? this.signedAngleDifference(options.target, current, options.period || 360) : options.target - current;
            const targetX = centerX + difference * width / fieldOfView;
            const targetColor = options.targetColor || "magenta";
            if(targetX >= left && targetX <= right)
                this.drawTriangle(targetX, baselineY + tickDirection * 2, upper ? "down" : "up", 4, targetColor);
            else
            {
                const edgeX = targetX < left ? left + 5 : right - 5;
                this.drawOutlinedText(targetX < left ? "◀" : "▶", edgeX, baselineY, {color:targetColor});
            }
        }
        this.canvas.restore();
    }

    drawVerticalTape(options)
    {
        const current = Number(options.current);
        if(!Number.isFinite(current))
            return;
        const top = Number(options.top);
        const bottom = Number(options.bottom);
        const centerY = (top + bottom) / 2;
        const height = bottom - top;
        const fieldOfView = Math.max(1, Number(options.fieldOfView) || 60);
        const minorStep = Math.max(0.1, Number(options.minorStep) || 5);
        const majorStep = Math.max(minorStep, Number(options.majorStep) || 15);
        const firstTick = Math.ceil((current - fieldOfView / 2) / minorStep) * minorStep;
        const lastTick = current + fieldOfView / 2;
        const x = Number(options.x);
        const color = options.color || "white";

        this.canvas.save();
        this.canvas.strokeStyle = color;
        this.canvas.lineWidth = Math.max(1, Number(this.parameters.line_width) || 1);
        this.canvas.beginPath();
        this.canvas.moveTo(x, top);
        this.canvas.lineTo(x, bottom);
        this.canvas.stroke();
        for(let value = firstTick; value <= lastTick + minorStep * 0.01; value += minorStep)
        {
            if(value < options.minimum - 1e-6 || value > options.maximum + 1e-6)
                continue;
            const y = centerY - (value - current) * height / fieldOfView;
            const major = this.isMajorTick(value, majorStep);
            this.canvas.beginPath();
            this.canvas.moveTo(x, y);
            this.canvas.lineTo(x - (major ? 10 : 5), y);
            this.canvas.stroke();
            if(major)
                this.drawOutlinedText(this.panLabel(value), x - 14, y, {color, align:"right", font:this.parameters.scale_font || "12px sans-serif"});
        }
        this.canvas.save();
        this.canvas.beginPath();
        this.canvas.moveTo(x, centerY);
        this.canvas.lineTo(x - 9, centerY - 6);
        this.canvas.lineTo(x - 9, centerY + 6);
        this.canvas.closePath();
        this.canvas.fillStyle = color;
        this.canvas.fill();
        this.canvas.restore();

        if(Number.isFinite(options.target))
        {
            const targetY = centerY - (options.target - current) * height / fieldOfView;
            if(targetY >= top && targetY <= bottom)
            {
                this.canvas.fillStyle = options.targetColor || "magenta";
                this.canvas.fillRect(x - 14, targetY - 2, 14, 4);
            }
        }
        this.canvas.restore();
    }

    drawReticle(centerX, centerY, panError, tiltError, panField, tiltField, width, height, color, targetColor)
    {
        this.canvas.save();
        this.canvas.strokeStyle = color;
        this.canvas.lineWidth = Math.max(1, Number(this.parameters.line_width) || 1);
        this.canvas.beginPath();
        this.canvas.moveTo(centerX - 10, centerY);
        this.canvas.lineTo(centerX + 10, centerY);
        this.canvas.moveTo(centerX, centerY - 10);
        this.canvas.lineTo(centerX, centerY + 10);
        this.canvas.stroke();
        if(Number.isFinite(panError) && Number.isFinite(tiltError))
        {
            const targetX = centerX + panError * width / Math.max(1, panField);
            const targetY = centerY - tiltError * height / Math.max(1, tiltField);
            this.canvas.strokeStyle = targetColor;
            this.canvas.strokeRect(targetX - 5, targetY - 5, 10, 10);
        }
        this.canvas.restore();
    }
}
