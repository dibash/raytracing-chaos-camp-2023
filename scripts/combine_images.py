from PIL import Image

# Define the input images and their file names
image_files = [
    "scene0.png",
    "scene1.png",
    "scene2.png",
    "scene3.png",
    "scene4.png",
    "scene5.png",
    "scene6.png",
    "scene7.png",
    "scene8.png"
]

# Open the input images
images = [Image.open(file) for file in image_files]

# Define the size of each image (assuming all images have the same size)
image_width, image_height = images[0].size

# Calculate the size of the output image
output_width = image_width * 3
output_height = image_height * 3

# Create a new image with the calculated size
output_image = Image.new("RGB", (output_width, output_height))

# Paste the input images onto the output image
for i in range(len(images)):
    x = (i % 3) * image_width
    y = (i // 3) * image_height
    output_image.paste(images[i], (x, y))

# Save the output image to a new file
output_image.save("combined_image.png")
