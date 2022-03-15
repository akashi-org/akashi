from akashi_core import ak, gl


@ak.entry()
def main():

    with ak.atom() as _:

        ak.rect(300, 300).ap(lambda h: (
            h.transform.pos(*ak.center()),
            h.shape.color(ak.Color.Red)
        ))
