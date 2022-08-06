package com.bithack.principia.shared;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.LinearLayout;

public class CustomLinearLayout extends LinearLayout {

    public static final int KEYBOARD_HIDDEN = 0;
    public static final int KEYBOARD_SHOWN = 1;

    private OnKeyboardStateChangedListener mListener = null;

    public interface OnKeyboardStateChangedListener {
        public void onKeyboardStateChanged(int new_state);
    }

    public CustomLinearLayout set_listener(OnKeyboardStateChangedListener l)
    {
        this.mListener = l;

        return this;
    }

    public CustomLinearLayout(Context context) {
        super(context);
    }

    public CustomLinearLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        final int proposedheight = MeasureSpec.getSize(heightMeasureSpec);
        final int actualHeight = getHeight();

        /* TODO: Make sure this event is only fired off once. */
        if (actualHeight > proposedheight){
            if (this.mListener != null) {
                this.mListener.onKeyboardStateChanged(this.KEYBOARD_SHOWN);
            }
        } else {
            if (this.mListener != null) {
                this.mListener.onKeyboardStateChanged(this.KEYBOARD_HIDDEN);
            }
        }

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

}
