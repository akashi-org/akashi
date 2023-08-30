from akashi_core import ak, gl


@ak.entry()
def main():

    with ak.rect(300, 300) as t:
        t.t_transform.pos(*ak.center())
        t.t_rect.fill_color(ak.Color.Red)
