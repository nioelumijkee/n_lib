""" Euklid rhythm's """
import pd

class EuklidClass:
    def __init__( self, *creation ):
        pd.verbose(0)
        self.step = 0
        self.hit = 0
        self.acc = 0
        self.rhythm = [0 for x in range(32)]
        self.w = 16.0
        self.h = 16.0
        self.wr = 16.0
        self.col_f = 22
        self.col_no = 0
        self.col_h = 11
        self.col_a = 13
        self.col_play = 17
        self.px0 = 4.0
        self.px1 = 12.0
        self.py0 = 4.0
        self.py1 = 12.0
        self.play_pos = 0
        return

    def float(self, f):
        if   f < 0:  f = 0
        elif f >= self.step: f = self.step-1
        self.play_pos = int(f)
        posx = self.play_pos * self.wr
        a = []
        a.append([self.rhythm[self.play_pos]])
        a.append(["disp", 3, 32, self.col_play, self.col_play, 1, 
                  posx + self.px0, self.py0, posx + self.px1, self.py1])
        t = tuple(a)
        return (t)

    def set_size(self, w, h):
        if   w < 0:   w = 0.0
        if   h < 0:   h = 0.0
        self.w = w
        self.h = h
        return

    def set_color(self, col_f, col_no, col_h, col_a, col_play):
        self.col_f    = col_f
        self.col_no   = col_no
        self.col_h    = col_h
        self.col_a    = col_a
        self.col_play = col_play
        return

    def set_step(self, x):
        x=int(x)
        if   x < 0:  x = 0
        elif x > 32: x = 32
        self.step = x
        return

    def set_hit(self, x):
        x=int(x)
        if   x < 0:  x = 0
        elif x > 32: x = 32
        self.hit = x
        return

    def set_acc(self, x):
        x=int(x)
        if   x < 0:  x = 0
        elif x > 32: x = 32
        self.acc = x
        self.make()
        return

    def make(self):
        # clip
        if self.hit > self.step: c_hit = self.step
        else:                    c_hit = self.hit
        if self.acc > c_hit:     c_acc = c_hit
        else:                    c_acc = self.acc

        # calc euklid
        n = 0
        for i in range(self.step):
            # hit
            j = i * c_hit
            j = j % self.step
            if j < c_hit:
                # acc
                k = n * c_acc
                k = k % c_hit
                if k < c_acc:
                    self.rhythm[i] = 2
                else:
                    self.rhythm[i] = 1
                n += 1
            else:
                self.rhythm[i] = 0
        
        #
        self.wr = self.w / self.step
        self.px0 = self.wr * 0.25
        self.px1 = self.wr * 0.75
        self.py0 = self.h * 0.25
        self.py1 = self.h * 0.75
        a = []
        a.append(["disp", "maxel", 33])
        for i in range(self.step):
            if self.rhythm[i] == 0:
                col = self.col_no
            elif self.rhythm[i] == 1:
                col = self.col_h
            else:
                col = self.col_a
            x0 = i  * self.wr
            x1 = x0 + self.wr
            a.append(["disp", 3, i, self.col_f, col, 1, x0, 0, x1, self.h])
        posx = self.play_pos * self.wr
        a.append(["disp", 3, 32, self.col_play, self.col_play, 1, 
                  posx + self.px0, self.py0, posx + self.px1, self.py1])
        t = tuple(a)
        return (t)
