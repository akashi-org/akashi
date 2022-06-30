from akashi_core import ak, gl


@ak.entry()
def main():

    ak.rect(300, 300, lambda t: (
        t.transform.pos(*ak.center()),
        t.shape.color(ak.Color.Red)
    ))
