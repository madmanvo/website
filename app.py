from flask import Flask, render_template, request, send_from_directory
import subprocess
import os

app = Flask(__name__)

def generate_dot(alph, nodes, initial, dead, final, transitions):
    alph_array = alph.split()
    nodes_array = [str(i) for i in range(int(nodes) + 1)]
    initial_node = initial.strip()
    final_nodes_array = final.split()

    if dead.strip():
        dead_node = dead.strip()
        nodes_array[int(dead_node)] = 'd'

    dot_graph = f"""
digraph DFA {{
    rankdir=LR;
    node [shape = circle]; 
    {initial_node} [label="{initial_node}"];
"""

    for node in nodes_array:
        if node != initial_node:
            if node in final_nodes_array:
                dot_graph += f'    {node} [label="{node}", shape=doublecircle];\n'
            else:
                dot_graph += f'    {node} [label="{node}"];\n'

    for node in transitions:
        for alphs in transitions[node]:
            target_node = transitions[node][alphs]
            dot_graph += f'    {node} -> {target_node} [label="{alphs}"];\n'

    dot_graph += "}\n"

    return dot_graph

def generate_tikz(dot_graph):
    try:
        result = subprocess.run(["dot2tex", "--autosize"], input=dot_graph, capture_output=True, text=True, check=True)
        tikz_graph = result.stdout
    except subprocess.CalledProcessError:
        tikz_graph = "Error: dot2tex command failed."
    except FileNotFoundError:
        tikz_graph = "Error: dot2tex is not installed or not found in PATH."
    
    return tikz_graph

@app.route('/')
def index():
    return render_template('index.html', tikz_graph='')

@app.route('/dfa.html', methods=['GET', 'POST'])
def dfa():
    if request.method == 'POST':
        alph = request.form['alph']
        nodes = request.form['nodes']
        initial = request.form['initial']
        dead = request.form['dead']
        final = request.form['final']
        transitions = {}

        for i in range(int(nodes) + 1):
            node = str(i)
            transitions[node] = {}
            for alph_char in alph.split():
                transitions[node][alph_char] = request.form.get(f'transition_{node}_{alph_char}', '')

        dot_graph = generate_dot(alph, nodes, initial, dead, final, transitions)
        tikz_graph = generate_tikz(dot_graph)
        return render_template('dfa.html', tikz_graph=tikz_graph)

    return render_template('dfa.html')

@app.route('/favicon.ico')
def favicon():
    return send_from_directory('static', 'favicon.ico')

if __name__ == '__main__':
    app.run(debug=True)

