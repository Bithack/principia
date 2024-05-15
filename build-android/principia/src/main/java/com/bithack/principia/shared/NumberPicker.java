/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.bithack.principia.shared;

// // stealing from github HeHe
// Stealing this class directly from the android internal namespace because why
// the hell didn't they ship with this public!?!?!?!

// Changed to allow negative numbers.

import java.util.Locale;

import android.content.Context;
import android.os.Handler;
import android.text.InputFilter;
import android.text.Spanned;
import android.text.method.NumberKeyListener;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.View.OnLongClickListener;
import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

import com.bithack.principia.R;

public class NumberPicker extends LinearLayout implements OnClickListener,
       OnFocusChangeListener, OnLongClickListener, OnEditorActionListener {

   public interface OnChangedListener {
       void onChanged(NumberPicker picker, int oldVal, int newVal);
   }

   public interface Formatter {
       String toString(int value);
   }

   /*
    * Use a custom NumberPicker formatting callback to use two-digit
    * minutes strings like "01".  Keeping a static formatter etc. is the
    * most efficient way to do this; it avoids creating temporary objects
    * on every call to format().
    */
   public static final NumberPicker.Formatter TWO_DIGIT_FORMATTER =
           new NumberPicker.Formatter() {
               final StringBuilder mBuilder = new StringBuilder();
               final java.util.Formatter mFmt = new java.util.Formatter(mBuilder);
               final Object[] mArgs = new Object[1];
               public String toString(int value) {
                   mArgs[0] = value;
                   mBuilder.delete(0, mBuilder.length());
                   mFmt.format("%02d", mArgs);
                   return mFmt.toString();
               }
       };

   private final Handler mHandler;
   private final Runnable mRunnable = new Runnable() {
       public void run() {
           if (mIncrement) {
               changeCurrent(mCurrent + mLongStep, mSlideUpInAnimation, mSlideUpOutAnimation);
               mHandler.postDelayed(this, mSpeed);
           } else if (mDecrement) {
               changeCurrent(mCurrent - mLongStep, mSlideDownInAnimation, mSlideDownOutAnimation);
               mHandler.postDelayed(this, mSpeed);
           }
       }
   };

   private final LayoutInflater mInflater;
   private final EditText mText;
   private final InputFilter mInputFilter;
   private final InputFilter mNumberInputFilter;

   private final Animation mSlideUpOutAnimation;
   private final Animation mSlideUpInAnimation;
   private final Animation mSlideDownOutAnimation;
   private final Animation mSlideDownInAnimation;

   private String[] mDisplayedValues;
   private int mStart;
   private int mEnd;
   private int mCurrent;
   private int mPrevious;
   private int mStep = 1;
   private int mLongStep = 5;
   private OnChangedListener mListener;
   private Formatter mFormatter;
   private long mSpeed = 300;

   private boolean mIncrement;
   private boolean mDecrement;

   public NumberPicker(Context context) {
       this(context, null);
   }

   public NumberPicker(Context context, AttributeSet attrs) {
       this(context, attrs, 0);
   }

   public NumberPicker(Context context, AttributeSet attrs,
           int defStyle) {
       super(context, attrs);
       setOrientation(VERTICAL);
       mInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
       mInflater.inflate(R.layout.number_picker, this, true);
       mHandler = new Handler();
       mInputFilter = new NumberPickerInputFilter();

       // TODO: Make which NumberRangeKeyListener it uses selected from |attrs|
       // if I ever make a Big Numbers Mode for Yugioh.
       //
       // mNumberInputFilter = new NumberRangeKeyListener();

       mNumberInputFilter = new NumberWithMinusRangeKeyListener();
       mIncrementButton = (NumberPickerButton) findViewById(R.id.increment);
       mIncrementButton.setOnClickListener(this);
       mIncrementButton.setOnLongClickListener(this);
       mIncrementButton.setNumberPicker(this);
       mDecrementButton = (NumberPickerButton) findViewById(R.id.decrement);
       mDecrementButton.setOnClickListener(this);
       mDecrementButton.setOnLongClickListener(this);
       mDecrementButton.setNumberPicker(this);

       mText = (EditText) findViewById(R.id.timepicker_input);
       mText.setOnFocusChangeListener(this);
       mText.setOnEditorActionListener(this);
       //mText.setFilters(new InputFilter[] { mInputFilter });
//       mText.setRawInputType(InputType.TYPE_CLASS_NUMBER);

       mSlideUpOutAnimation = new TranslateAnimation(
               Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF,
               0, Animation.RELATIVE_TO_SELF, 0,
               Animation.RELATIVE_TO_SELF, -100);
       mSlideUpOutAnimation.setDuration(200);
       mSlideUpInAnimation = new TranslateAnimation(
               Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF,
               0, Animation.RELATIVE_TO_SELF, 100,
               Animation.RELATIVE_TO_SELF, 0);
       mSlideUpInAnimation.setDuration(200);
       mSlideDownOutAnimation = new TranslateAnimation(
               Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF,
               0, Animation.RELATIVE_TO_SELF, 0,
               Animation.RELATIVE_TO_SELF, 100);
       mSlideDownOutAnimation.setDuration(200);
       mSlideDownInAnimation = new TranslateAnimation(
               Animation.RELATIVE_TO_SELF, 0, Animation.RELATIVE_TO_SELF,
               0, Animation.RELATIVE_TO_SELF, -100,
               Animation.RELATIVE_TO_SELF, 0);
       mSlideDownInAnimation.setDuration(200);

       if (!isEnabled()) {
           setEnabled(false);
       }
   }

   @Override
   public void setEnabled(boolean enabled) {
       super.setEnabled(enabled);
       mIncrementButton.setEnabled(enabled);
       mDecrementButton.setEnabled(enabled);
       mText.setEnabled(enabled);
   }

   public void setOnChangeListener(OnChangedListener listener) {
       mListener = listener;
   }

   public void setFormatter(Formatter formatter) {
       mFormatter = formatter;
   }

   /**
    * Set the range of numbers allowed for the number picker. The current
    * value will be automatically set to the start.
    *
    * @param start the start of the range (inclusive)
    * @param end the end of the range (inclusive)
    */
   public void setRange(int start, int end) {
       mStart = start;
       mEnd = end;
       mCurrent = start;
       updateView();
   }

   /**
    * The amount the value with increment or decrement each time one of the two buttons are pressed.
    * @param step the number of steps to increment/decrement.
    */
   public void setStep(int step)
   {
       mStep = step;
       updateView();
   }

   public void setLongStep(int longStep)
   {
       mLongStep = longStep;
       updateView();
   }

   /**
    * Set the range of numbers allowed for the number picker. The current
    * value will be automatically set to the start. Also provide a mapping
    * for values used to display to the user.
    *
    * @param start the start of the range (inclusive)
    * @param end the end of the range (inclusive)
    * @param displayedValues the values displayed to the user.
    */
   public void setRange(int start, int end, String[] displayedValues) {
       mDisplayedValues = displayedValues;
       mStart = start;
       mEnd = end;
       mCurrent = start;
       updateView();
   }

   public void setCurrent(int current) {
       mCurrent = current;
       updateView();
   }

   public void setValue(int value) {
       this.setCurrent(value);
   }

   /**
    * The speed (in milliseconds) at which the numbers will scroll
    * when the the +/- buttons are longpressed. Default is 300ms.
    */
   public void setSpeed(long speed) {
       mSpeed = speed;
   }

   public void onClick(View v) {

       /* The text view may still have focus so clear it's focus which will
        * trigger the on focus changed and any typed values to be pulled.
        */
       mText.clearFocus();

       // now perform the increment/decrement
       if (R.id.increment == v.getId()) {
           changeCurrent(mCurrent + mStep, mSlideUpInAnimation, mSlideUpOutAnimation);
       } else if (R.id.decrement == v.getId()) {
           changeCurrent(mCurrent - mStep, mSlideDownInAnimation, mSlideDownOutAnimation);
       }
   }

   private String formatNumber(int value) {
       return (mFormatter != null)
               ? mFormatter.toString(value)
               : String.valueOf(value);
   }

   private void changeCurrent(int current, Animation in, Animation out) {

       // Wrap around the values if we go past the start or end
       if (current > mEnd) {
           current = mStart;
       } else if (current < mStart) {
           current = mEnd;
       }
       mPrevious = mCurrent;
       mCurrent = current;
       notifyChange();
       updateView();
   }

   private void notifyChange() {
       if (mListener != null) {
           mListener.onChanged(this, mPrevious, mCurrent);
       }
   }

   private void updateView() {

       /* If we don't have displayed values then use the
        * current number else find the correct value in the
        * displayed values for the current number.
        */
       if (mDisplayedValues == null) {
           mText.setText(formatNumber(mCurrent));
       } else {
           mText.setText(mDisplayedValues[mCurrent - mStart]);
       }
   }

   private void validateCurrentView(CharSequence str) {
       int val = getSelectedPos(str.toString());
       if ((val >= mStart) && (val <= mEnd)) {
           mPrevious = mCurrent;
           mCurrent = val;
           notifyChange();
       }
       updateView();
   }

   public void onFocusChange(View v, boolean hasFocus) {

       /* When focus is lost check that the text field
        * has valid values.
        */
       if (!hasFocus) {
           String str = String.valueOf(((EditText) v).getText());
           if ("".equals(str)) {
               // Restore to the old value as we don't allow empty values
               updateView();
           } else {
               // Check the new value and ensure it's in range
               validateCurrentView(str);
           }
       }
   }

   /**
    * We start the long click here but rely on the {@link NumberPickerButton}
    * to inform us when the long click has ended.
    */
   public boolean onLongClick(View v) {

       /* The text view may still have focus so clear it's focus which will
        * trigger the on focus changed and any typed values to be pulled.
        */
       mText.clearFocus();

       if (R.id.increment == v.getId()) {
           mIncrement = true;
           mHandler.post(mRunnable);
       } else if (R.id.decrement == v.getId()) {
           mDecrement = true;
           mHandler.post(mRunnable);
       }
       return true;
   }

   public void cancelIncrement() {
       mIncrement = false;
   }

   public void cancelDecrement() {
       mDecrement = false;
   }

   private static final char[] DIGIT_CHARACTERS = new char[] {
       '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
   };

   private static final char[] DIGIT_CHARACTERS_WITH_MINUS = new char[] {
       '-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
   };

   private NumberPickerButton mIncrementButton;
   private NumberPickerButton mDecrementButton;

   private class NumberPickerInputFilter implements InputFilter {
       public CharSequence filter(CharSequence source, int start, int end,
               Spanned dest, int dstart, int dend) {
           if (mDisplayedValues == null) {
               return mNumberInputFilter.filter(source, start, end, dest, dstart, dend);
           }
           CharSequence filtered = String.valueOf(source.subSequence(start, end));
           String result = String.valueOf(dest.subSequence(0, dstart))
                   + filtered
                   + dest.subSequence(dend, dest.length());
           String str = result.toLowerCase(Locale.US);
           for (String val : mDisplayedValues) {
               val = val.toLowerCase(Locale.US);
               if (val.startsWith(str)) {
                   return filtered;
               }
           }
           return "";
       }
   }

   private class NumberRangeKeyListener extends NumberKeyListener {

       // XXX This doesn't allow for range limits when controlled by a
       // soft input method!
       public int getInputType() {
           return 0x00000002; // InputType.TYPE_CLASS_NUMBER;
       }

       @Override
       protected char[] getAcceptedChars() {
           return DIGIT_CHARACTERS;
       }

       @Override
       public CharSequence filter(CharSequence source, int start, int end,
               Spanned dest, int dstart, int dend) {

           CharSequence filtered = super.filter(source, start, end, dest, dstart, dend);
           if (filtered == null) {
               filtered = source.subSequence(start, end);
           }

           String result = String.valueOf(dest.subSequence(0, dstart))
                   + filtered
                   + dest.subSequence(dend, dest.length());

           if ("".equals(result)) {
               return result;
           }
           int val = getSelectedPos(result);

           /* Ensure the user can't type in a value greater
            * than the max allowed. We have to allow less than min
            * as the user might want to delete some numbers
            * and then type a new number.
            */
           if (val > mEnd) {
               return "";
           } else {
               return filtered;
           }
       }
   }

   private class NumberWithMinusRangeKeyListener extends NumberRangeKeyListener {
       @Override
       protected char[] getAcceptedChars() {
           return DIGIT_CHARACTERS_WITH_MINUS;
       }
   }

   private int getSelectedPos(String str) {
       if (mDisplayedValues == null) {
           // CHANGE: Accept the '-' raw!
           // ASSUMPTION: Any NumberFormatException will be because of a raw '-'
           // because that's the only non-numeric character allowed into this
           // method.
           try {
              return Integer.parseInt(str);
           } catch (NumberFormatException e) {
               // Pathological case: If they enter "5-2", we'll interpret that
               // mess as 0. I dare you to care.
               return 0;
           }
       } else {
           for (int i = 0; i < mDisplayedValues.length; i++) {

               /* Don't force the user to type in jan when ja will do */
               str = str.toLowerCase(Locale.US);
               if (mDisplayedValues[i].toLowerCase(Locale.US).startsWith(str)) {
                   return mStart + i;
               }
           }

           /* The user might have typed in a number into the month field i.e.
            * 10 instead of OCT so support that too.
            */
           try {
               return Integer.parseInt(str);
           } catch (NumberFormatException e) {

               /* Ignore as if it's not a number we don't care */
           }
       }
       return mStart;
   }

   /**
    * @return the current value.
    */
   public int getCurrent() {
       return mCurrent;
   }

   public int getValue() {
       return this.getCurrent();
   }

    @Override
    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (event != null) {
            // if shift key is down, then we want to insert the '\n' char in the TextView;
            // otherwise, the default action is to send the message.
            return true;
        }

       String str = String.valueOf(((EditText) v).getText());
       if ("".equals(str)) {
           // Restore to the old value as we don't allow empty values
           updateView();
       } else {
           // Check the new value and ensure it's in range
           validateCurrentView(str);
       }

       return false;
    }
}

