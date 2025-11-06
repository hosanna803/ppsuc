import pygame
import random
import sys

# Initialize pygame
pygame.init()

# Screen dimensions
WIDTH = 400
HEIGHT = 600
FPS = 60

# Colors
WHITE = (255, 255, 255)
BLUE = (135, 206, 250)
GREEN = (0, 200, 0)
YELLOW = (255, 255, 0)

# Game window
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Flappy Bird")
clock = pygame.time.Clock()

# Fonts
font = pygame.font.SysFont("Arial", 24)

# Bird properties
bird_x = 100
bird_y = HEIGHT // 2
bird_radius = 15  
bird_vel = 0
gravity = 0.5
jump = -8

# Pipes
pipe_width = 50
pipe_gap = 150
pipe_list = []
pipe_speed = 3

# Score
score = 0

def draw_bird(x, y):
    pygame.draw.circle(screen, YELLOW, (x, int(y)), bird_radius)

def create_pipe():
    gap_y = random.randint(100, HEIGHT - 100)
    top_rect = pygame.Rect(WIDTH, 0, pipe_width, gap_y - pipe_gap // 2)
    bottom_rect = pygame.Rect(WIDTH, gap_y + pipe_gap // 2, pipe_width, HEIGHT)
    return top_rect, bottom_rect

def draw_pipes(pipes):
    for pipe in pipes:
        pygame.draw.rect(screen, GREEN, pipe)

def check_collision(pipes, y):
    if y - bird_radius <= 0 or y + bird_radius >= HEIGHT:
        return True
    for pipe in pipes:
        if pipe.colliderect(pygame.Rect(bird_x - bird_radius, y - bird_radius, bird_radius*2, bird_radius*2)):
            return True
    return False

# Main game loop
running = True
frame_count = 0
while running:
    clock.tick(FPS)
    screen.fill(BLUE)

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_SPACE:
                bird_vel = jump

    # Bird movement
    bird_vel += gravity
    bird_y += bird_vel

    # Pipes
    frame_count += 1
    if frame_count > 90:
        frame_count = 0
        pipe_list.extend(create_pipe())

    pipe_list = [p.move(-pipe_speed, 0) for p in pipe_list if p.x + pipe_width > 0]

    # Collision detection
    if check_collision(pipe_list, bird_y):
        text = font.render(f"Game Over! Final Score: {score}", True, (255, 0, 0))
        screen.blit(text, (50, HEIGHT // 2))
        pygame.display.flip()
        pygame.time.wait(2000)
        pygame.quit()
        sys.exit()

    # Scoring
    for pipe in pipe_list:
        if pipe.x + pipe_width == bird_x:
            score += 0.5  # top and bottom count together → +1 total

    # Draw everything
    draw_bird(bird_x, bird_y)
    draw_pipes(pipe_list)
    score_text = font.render(f"Score: {int(score)}", True, WHITE)
    screen.blit(score_text, (10, 10))

    pygame.display.flip()

pygame.quit()
