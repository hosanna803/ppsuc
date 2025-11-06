import turtle
import time

# Setup
screen = turtle.Screen()
screen.bgcolor("white")
screen.title("Doraemon Animation")
t = turtle.Turtle()
t.speed(5)

# Helper functions
def draw_circle(color, x, y, radius):
    t.penup()
    t.goto(x, y - radius)
    t.pendown()
    t.fillcolor(color)
    t.begin_fill()
    t.circle(radius)
    t.end_fill()

def draw_rectangle(color, x, y, width, height):
    t.penup()
    t.goto(x, y)
    t.pendown()
    t.fillcolor(color)
    t.begin_fill()
    for _ in range(2):
        t.forward(width)
        t.right(90)
        t.forward(height)
        t.right(90)
    t.end_fill()

# Doraemon head
draw_circle("deepskyblue", 0, 0, 120)
draw_circle("white", 0, -10, 100)

# Face
draw_circle("white", 0, 20, 80)

# Eyes
draw_circle("white", -30, 80, 20)
draw_circle("white", 30, 80, 20)
draw_circle("black", -30, 80, 10)
draw_circle("black", 30, 80, 10)

# Nose
draw_circle("red", 0, 60, 15)

# Mouth
t.penup()
t.goto(-40, 40)
t.setheading(-60)
t.pendown()
t.width(3)
t.circle(50, 120)

# Body
draw_rectangle("deepskyblue", -60, -120, 120, 150)
draw_circle("white", 0, -120, 60)

# Collar
draw_rectangle("red", -60, 30, 120, 10)

# Bell
draw_circle("yellow", 0, 20, 10)
draw_circle("black", 0, 20, 5)

# Animation: Doraemon blinking eyes & waving hand
def blink_and_wave():
    for _ in range(3):
        # Blink (close eyes)
        draw_circle("white", -30, 80, 20)
        draw_circle("white", 30, 80, 20)
        time.sleep(0.2)

        draw_circle("black", -30, 80, 10)
        draw_circle("black", 30, 80, 10)
        time.sleep(0.2)

    # Wave hand
    t.width(10)
    t.pencolor("deepskyblue")
    for angle in [30, -30] * 3:
        t.penup()
        t.goto(60, -30)
        t.setheading(angle)
        t.pendown()
        t.forward(60)
        time.sleep(0.3)
        t.undo()

# Run animation
blink_and_wave()

t.hideturtle()
screen.mainloop()
