import tkinter as tk


ROD_LENGTH = 2  # meters
NUM_LEDS = 200
IMAGE_HEIGHT = 50

ZOOM = 2
CANVAS_WIDTH = NUM_LEDS * 1 * ZOOM
CANVAS_HEIGHT = IMAGE_HEIGHT * 1 * ZOOM

root = tk.Tk()
root.geometry(f'{600}x{400}')
root.title('Canvas')

canvas = tk.Canvas(root, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg='white')
canvas.pack()


def drawCircle(x_center, y_center):
    radius = 50

    x1 = x_center - radius
    y1 = y_center - radius
    x2 = x_center + radius
    y2 = y_center + radius
    circle = canvas.create_oval(x1, y1, x2, y2, fill="red")


print(f'{CANVAS_WIDTH}')
drawCircle(CANVAS_WIDTH/2, CANVAS_HEIGHT/2)

root.mainloop()
