
static struct
{
    int r, g, b;
}
pc2rgb[] =
{
    {  0,   0,   0},  // black
    {  0,   0, 128},  // blue
    {  0, 128,   0},  // green
    {  0, 128, 128},  // cyan
    {128,   0,   0},  // red
    {128,   0, 128},  // magenta
    {128, 128,   0},  // brown
    {192, 192, 192},  // gray
    {160, 160, 160},  // dark gray
    {  0,   0, 255},  // bright blue
    {  0, 255,   0},  // bright green
    {  0, 255, 255},  // bright cyan
    {255,   0,   0},  // bright red
    {255,   0, 255},  // bright magenta
    {255, 255,   0},  // yellow
    {255, 255, 255},  // white
};

