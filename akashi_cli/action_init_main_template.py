from akashi_core import ak, gl


@ak.entry()
def main():

    ak.rect(300, 300, lambda h: (
        h.transform.pos(*ak.center()),
        h.shape.color(ak.Color.Red)
    ))
