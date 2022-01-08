from akashi_core import ak, gl


@ak.entry()
def main():

    with ak.atom() as _:

        with ak.lane():

            ak.circle(100).ap(
                lambda h: h.pos(*ak.center()),
                lambda h: h.color(ak.Color.Red)
            )