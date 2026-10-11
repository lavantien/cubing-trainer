import sqlite3

c = sqlite3.connect("build/test_e2e_script/p6148/cubing-trainer/trainer.db")
for k, v in c.execute("select key, value from settings order by key"):
    print(k, "=", v)
