
"""
define %setPixel $X $Y $V as
	$_pos = ($Y * $SCREEN_WIDTH);
	$_pos = ($_pos + $X);

	rOP = $V; //Write value into screen area of RAM.
	RAMwrite #xE $_pos;
end


define %toRGB332 $R $G $B $result as
	//Takes range 0x00 to 0xFF for each channel.
	$result = ($R & #xE0); //First 3 bits are red.

	//Next 3 bits are green.
	$G >> #3;
	rOP & #x1C;
	$result = ($result | rOP);

	//Final 2 are blue.
	$B >> #6;
	rOP & #x03;
	$result = ($result | rOP);
end
"""


SCREEN_WIDTH = 32;
SCREEN_HEIGHT = 16;

def setPixel(X, Y, V) -> None:
	pos = (y * SCREEN_WIDTH)
	pos = (pos + X)

	print(pos)


def toRGB332(R, G, B) -> int:
	result = R & 0xE0
	
	g = G >> 3
	g = g & 0x1C
	result = result | g
	
	b = B >> 6
	b = b & 0x03
	result = result | b

	return result


print(f"{toRGB332(0xFF, 0x00, 0xFF):08b}")