![Logo](/Source/logo.png)
## Bitcrusher plug-in with bytebeat functionality.

![Screenshot](/Screenshots/screenshot.png)

Bytebeats are short, usually one-line C-style formulas consisting of arithmetic and bit operations, integers and an increasing index *t*. Only 8 least significant bits are masked off from the output, which creates interesting audio effects including rhythms and melodies.

### Plug-in features
- Toggling **dither** changes the sound characteristics on low bit depths. Mainly this adds some noise.
- When **Mask output to 8 bits** is enabled, the output of the bytebeat formula is wrapped to 8 bits. This produces a classic bytebeat sound. Note, that since the output is now 8-bit, the bit depth slider has little effect on the output.
- **Bit depth** and **sample rate** reduction produce a traditional bitcrush effect. 
    - If you want to just use the classic bitcrusher features with no additional distortion from the bytebeat generation, tick off "mask output to 8 bits" and write "x" to the text editor.
- The **left-shift** slider bitshifts the outgoing audio sample mapped to an integer range by the specified amount. **Adjusting this slider can increase the gain of the signal, so use discretion!** 

### More resources on bytebeat
- [In depth information and example formulas](https://countercomplex.blogspot.com/2011/10/some-deep-analysis-of-one-line-music.html)
- [Online player and lots of examle formulas](https://dollchan.net/bytebeat/#4AAAA+kUzNNDSKLGzM68pqQFSZpraJkC+GpBpaAwRAAA)
- [Paper by Ville-Matias Heikkilä](https://arxiv.org/abs/1112.1368)

### Guide for writing bytebeat expressions
You can include variables *x* (audio input) and *t* (bytebeat increasing index), integers and operators listed below in your formulas. 
Variable *x* is the current input sample mapped to a [0,255] range. You can use this variable in the expression along with *t* to merge the input audio signal to your formula or use only *t*.
- If you get no wet signal from your formula, try setting *Mask output to 8 bits* on. 
- Simply adding +x to a t-formula will give some interesting results.
- Decreasing the sample rate slows down the looping speed of the formula.

Operators supported:
- Functions sin()/cos()
- Bitwise negation ~
- Multiplication, division, modulus *, /, %
- Addition, subtractionn (unary negation not supported) +, -
- Left/right bit shift <<, >>
- Less/greater than (or equal to) <, <=, >, >=
- (not) equal to ==, !=
- Bitwise AND &"
- Bitwise exclusive OR ^
- Bitwise inclusive OR |

Whitespaces are bypassed, so both `x+t&x` and `x + t & x` are valid.

### Example formulas

- `x+(t&t>>12)*(t>>4|t>>8)`
- `t*(42&t>>10)+x*2`
- `x+t`
- `x+t&x`
- `x+sin(t)+t&t<<8`
- `(t*(4|7&t>>13)>>(~t>>11&1)&128)+(t*(t>>11&t>>13)*(~t>>9&3)&64)`

### TODO:

- Support for ternary operator ? :
- Support for unary negation