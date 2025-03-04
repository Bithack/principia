package com.bithack.principia.shared;

import android.content.Context;
import android.os.Handler;
import android.os.SystemClock;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.util.AttributeSet;
import android.util.Log;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

public class CodeEditor extends androidx.appcompat.widget.AppCompatEditText

{
    public interface OnTextChangedListener
    {
        public void onTextChanged( String text );
    }

    public OnTextChangedListener onTextChangedListener = null;
    public final int updateDelay = 5000;
    public boolean dirty = false;
    public long last_edit = 0;

    private static final int COLOR_NUMBER = 0xff7ba212;
    private static final int COLOR_KEYWORD = 0xff399ed7;
    private static final int COLOR_BUILTIN = 0xffd79e39;
    private static final int COLOR_COMMENT = 0xff808080;

    private static final Pattern numbers = Pattern.compile(
        "\\b(\\d*[.]?\\d+)\\b" );
    private static final Pattern keywords = Pattern.compile(
        "\\b("+
        "do|for|while|if|then|else|in|out|end|true|false|"+
        "function|return|self|local"+
        ")\\b" );
    private static final Pattern builtins = Pattern.compile(
        "\\b("+
        "this|game|cam|world|math|pairs|ipairs|setmetatable|nil"+
        ")\\b" );
    private static final Pattern comments = Pattern.compile(
        "--\\[\\[(?:.|[\\n\\r])*?--\\]\\]|--.*" );
    private static final Pattern trailingWhiteSpace = Pattern.compile(
        "[\\t ]+$",
        Pattern.MULTILINE );

    private static final Handler updateHandler = new Handler() {
    };
    private final Runnable updateRunnable =
        new Runnable()
        {
            @Override
            public void run()
            {
                if (SystemClock.uptimeMillis()-updateDelay < last_edit) {
                    Log.v("Principia", "skipping!");
                    return;
                }
                Editable e = getText();

                if( onTextChangedListener != null )
                    onTextChangedListener.onTextChanged( e.toString() );

                highlightWithoutChange( e );
                Log.v("Principia", "runnable run end");
            }
        };
    private boolean modified = true;

    public CodeEditor( Context context )
    {
        super( context );
        init();
    }

    public CodeEditor( Context context, AttributeSet attrs )
    {
        super( context, attrs );
        init();
    }

    public void setTextHighlighted( CharSequence text )
    {
        cancelUpdate();

        dirty = false;

        modified = false;
        setText( highlight( new SpannableStringBuilder( text ) ) );
        modified = true;

        if( onTextChangedListener != null )
            onTextChangedListener.onTextChanged( text.toString() );
    }

    public String getCleanText()
    {
        return trailingWhiteSpace
            .matcher( getText() )
            .replaceAll( "" );
    }

    public void refresh()
    {
        highlightWithoutChange( getText() );
    }

    private void init()
    {
        setHorizontallyScrolling( true );

        setFilters( new InputFilter[]{
            new InputFilter()
            {
                @Override
                public CharSequence filter(
                    CharSequence source,
                    int start,
                    int end,
                    Spanned dest,
                    int dstart,
                    int dend )
                {
                    if( modified &&
                        end-start == 1 &&
                        start < source.length() &&
                        dstart < dest.length() )
                    {
                        char c = source.charAt( start );

                        if( c == '\n' )
                            return autoIndent(
                                source,
                                start,
                                end,
                                dest,
                                dstart,
                                dend );
                    }

                    return source;
                }
            } } );

        addTextChangedListener(
            new TextWatcher()
            {
                public void onTextChanged(
                    CharSequence s,
                    int start,
                    int before,
                    int count )
                {
                }

                public void beforeTextChanged(
                    CharSequence s,
                    int start,
                    int count,
                    int after )
                {
                }

                public void afterTextChanged( Editable e )
                {
                    last_edit = SystemClock.uptimeMillis();
                    Log.v("Principia", "afterTextChanged begin");
                    cancelUpdate();

                    if( !modified ) {
                        Log.v("Principia", "afterTextChanged end");
                        return;
                    }

                    dirty = true;
                    updateHandler.postDelayed(
                        updateRunnable,
                        updateDelay );
                    Log.v("Principia", "afterTextChanged end");
                }
            } );
    }

    private void cancelUpdate()
    {
        updateHandler.removeCallbacks( updateRunnable );
    }

    private void highlightWithoutChange( Editable e )
    {
        modified = false;
        highlight( e );
        modified = true;
    }

    private Editable highlight( Editable e )
    {
        last_edit = SystemClock.uptimeMillis();

        Log.v("Principia", "highlight begin");
        try
        {
            // don't use e.clearSpans() because it will remove
            // too much
            clearSpans( e );

            if( e.length() == 0 )
                return e;

            /*
            for( Matcher m = numbers.matcher( e );
                m.find(); )
                e.setSpan(
                    new ForegroundColorSpan( COLOR_NUMBER ),
                    m.start(),
                    m.end(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE );
                    */

            for( Matcher m = keywords.matcher( e );
                m.find(); )
                e.setSpan(
                    new ForegroundColorSpan( COLOR_KEYWORD ),
                    m.start(),
                    m.end(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE );

            for( Matcher m = builtins.matcher( e );
                m.find(); )
                e.setSpan(
                    new ForegroundColorSpan( COLOR_BUILTIN ),
                    m.start(),
                    m.end(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE );

            for( Matcher m = comments.matcher( e );
                m.find(); )
                e.setSpan(
                    new ForegroundColorSpan( COLOR_COMMENT ),
                    m.start(),
                    m.end(),
                    Spannable.SPAN_EXCLUSIVE_EXCLUSIVE );
        }
        catch( Exception ex )
        {
        }

        Log.v("Principia", "highlight end");
        return e;
    }

    private void clearSpans( Editable e )
    {
        // remove foreground color spans
        {
            ForegroundColorSpan[] spans = e.getSpans(
                0,
                e.length(),
                ForegroundColorSpan.class );

            for( int n = spans.length; n-- > 0; )
                e.removeSpan( spans[n] );
        }

        /*
        // remove background color spans
        {
            BackgroundColorSpan spans[] = e.getSpans(
                0,
                e.length(),
                BackgroundColorSpan.class );

            for( int n = spans.length; n-- > 0; )
                e.removeSpan( spans[n] );
        }
        */
    }

    private CharSequence autoIndent(
        CharSequence source,
        int start,
        int end,
        Spanned dest,
        int dstart,
        int dend )
    {
        String indent = "";
        int istart = dstart-1;
        int iend = -1;

        // find start of this line
        boolean dataBefore = false;
        int pt = 0;

        for( ; istart > -1; --istart )
        {
            char c = dest.charAt( istart );

            if( c == '\n' )
                break;

            if( c != ' ' &&
                c != '\t' )
            {
                if( !dataBefore )
                {
                    // indent always after those characters
                    if( c == '{' ||
                        c == '+' ||
                        c == '-' ||
                        c == '*' ||
                        c == '/' ||
                        c == '%' ||
                        c == '^' ||
                        c == '=' )
                        --pt;

                    dataBefore = true;
                }

                // parenthesis counter
                if( c == '(' )
                    --pt;
                else if( c == ')' )
                    ++pt;
            }
        }

        // copy indent of this line into the next
        if( istart > -1 )
        {
            char charAtCursor = dest.charAt( dstart );

            for( iend = ++istart;
                iend < dend;
                ++iend )
            {
                char c = dest.charAt( iend );

                // auto expand comments
                if( charAtCursor != '\n' &&
                    c == '/' &&
                    iend+1 < dend &&
                    dest.charAt( iend ) == c )
                {
                    iend += 2;
                    break;
                }

                if( c != ' ' &&
                    c != '\t' )
                    break;
            }

            indent += dest.subSequence( istart, iend );
        }

        // add new indent
        if( pt < 0 )
            indent += "\t";

        // append white space of previous line and new indent
        return source+indent;
    }
}
